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
 *  071704 AC   Creation
 */

   /**
    * Number of leading input bars {@link Core#CDLLONGLINE} consumes before it
    * can produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int CDLLONGLINE_Lookback( )
   {
      int BodyLong_rangeType = this.candleSettings[CandleSettingType.BodyLong.ordinal()].rangeType.ordinal();
      int BodyLong_avgPeriod = this.candleSettings[CandleSettingType.BodyLong.ordinal()].avgPeriod;
      double BodyLong_factor = this.candleSettings[CandleSettingType.BodyLong.ordinal()].factor;
      int ShadowShort_rangeType = this.candleSettings[CandleSettingType.ShadowShort.ordinal()].rangeType.ordinal();
      int ShadowShort_avgPeriod = this.candleSettings[CandleSettingType.ShadowShort.ordinal()].avgPeriod;
      double ShadowShort_factor = this.candleSettings[CandleSettingType.ShadowShort.ordinal()].factor;
      return Math.max(BodyLong_avgPeriod, ShadowShort_avgPeriod) ;

   }
   RetCode CDLLONGLINE_Internal( int startIdx,
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
      int BodyLong_rangeType = this.candleSettings[CandleSettingType.BodyLong.ordinal()].rangeType.ordinal();
      int BodyLong_avgPeriod = this.candleSettings[CandleSettingType.BodyLong.ordinal()].avgPeriod;
      double BodyLong_factor = this.candleSettings[CandleSettingType.BodyLong.ordinal()].factor;
      int ShadowShort_rangeType = this.candleSettings[CandleSettingType.ShadowShort.ordinal()].rangeType.ordinal();
      int ShadowShort_avgPeriod = this.candleSettings[CandleSettingType.ShadowShort.ordinal()].avgPeriod;
      double ShadowShort_factor = this.candleSettings[CandleSettingType.ShadowShort.ordinal()].factor;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = CDLLONGLINE_Lookback();
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
      BodyTrailingIdx = startIdx - BodyLong_avgPeriod;
      ShadowPeriodTotal = 0;
      ShadowTrailingIdx = startIdx - ShadowShort_avgPeriod;
      i = BodyTrailingIdx;
      while( i < startIdx ) {
         BodyPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyLong_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)));
         i += 1;
      }
      i = ShadowTrailingIdx;
      while( i < startIdx ) {
         ShadowPeriodTotal += ((ShadowShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowShort_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)));
         i += 1;
      }
      /* Proceed with the calculation for the requested range.
       * Must have:
       * - long real body
       * - short upper and lower shadow
       * The meaning of "long" and "short" is specified with TA_SetCandleSettings
       * outInteger is positive (1 to 100) when white (bullish), negative (-1 to -100) when black (bearish)
       */
      outIdx = 0;
      do {
         if( Math.abs(inClose[i] - inOpen[i]) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (BodyPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyLong_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && (inHigh[i] - ((inClose[i] >= inOpen[i]) ? inClose[i] : inOpen[i])) < ((ShadowShort_factor * (((ShadowShort_avgPeriod != 0) ? (ShadowPeriodTotal / ShadowShort_avgPeriod) : ((ShadowShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowShort_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)))) / ((ShadowShort_rangeType == 2) ? 2.0 : 1.0)))) && (((inClose[i] >= inOpen[i]) ? inOpen[i] : inClose[i]) - inLow[i]) < ((ShadowShort_factor * (((ShadowShort_avgPeriod != 0) ? (ShadowPeriodTotal / ShadowShort_avgPeriod) : ((ShadowShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowShort_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)))) / ((ShadowShort_rangeType == 2) ? 2.0 : 1.0)))) ) {
            outInteger[outIdx++] = ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) * 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         /* add the current range and subtract the first range: this is done after the pattern recognition
          * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
          */
         BodyPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyLong_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0))) - ((BodyLong_rangeType == 0) ? (Math.abs(inClose[BodyTrailingIdx] - inOpen[BodyTrailingIdx])) : ((BodyLong_rangeType == 1) ? (inHigh[BodyTrailingIdx] - inLow[BodyTrailingIdx]) : ((BodyLong_rangeType == 2) ? ((inHigh[BodyTrailingIdx] - inLow[BodyTrailingIdx]) - Math.abs(inClose[BodyTrailingIdx] - inOpen[BodyTrailingIdx])) : 0.0)));
         ShadowPeriodTotal += ((ShadowShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowShort_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0))) - ((ShadowShort_rangeType == 0) ? (Math.abs(inClose[ShadowTrailingIdx] - inOpen[ShadowTrailingIdx])) : ((ShadowShort_rangeType == 1) ? (inHigh[ShadowTrailingIdx] - inLow[ShadowTrailingIdx]) : ((ShadowShort_rangeType == 2) ? ((inHigh[ShadowTrailingIdx] - inLow[ShadowTrailingIdx]) - Math.abs(inClose[ShadowTrailingIdx] - inOpen[ShadowTrailingIdx])) : 0.0)));
         i += 1;
         BodyTrailingIdx += 1;
         ShadowTrailingIdx += 1;
      } while( i <= endIdx );
      /* All done. Indicate the output limits and return. */
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   RetCode CDLLONGLINE_Internal( int startIdx,
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
      int BodyLong_rangeType = this.candleSettings[CandleSettingType.BodyLong.ordinal()].rangeType.ordinal();
      int BodyLong_avgPeriod = this.candleSettings[CandleSettingType.BodyLong.ordinal()].avgPeriod;
      double BodyLong_factor = this.candleSettings[CandleSettingType.BodyLong.ordinal()].factor;
      int ShadowShort_rangeType = this.candleSettings[CandleSettingType.ShadowShort.ordinal()].rangeType.ordinal();
      int ShadowShort_avgPeriod = this.candleSettings[CandleSettingType.ShadowShort.ordinal()].avgPeriod;
      double ShadowShort_factor = this.candleSettings[CandleSettingType.ShadowShort.ordinal()].factor;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      lookbackTotal = CDLLONGLINE_Lookback();
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      BodyPeriodTotal = 0;
      BodyTrailingIdx = startIdx - BodyLong_avgPeriod;
      ShadowPeriodTotal = 0;
      ShadowTrailingIdx = startIdx - ShadowShort_avgPeriod;
      i = BodyTrailingIdx;
      while( i < startIdx ) {
         BodyPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)));
         i += 1;
      }
      i = ShadowTrailingIdx;
      while( i < startIdx ) {
         ShadowPeriodTotal += ((ShadowShort_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((ShadowShort_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((ShadowShort_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)));
         i += 1;
      }
      outIdx = 0;
      do {
         if( Math.abs((double)inClose[i] - (double)inOpen[i]) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (BodyPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && ((double)inHigh[i] - (((double)inClose[i] >= (double)inOpen[i]) ? (double)inClose[i] : (double)inOpen[i])) < ((ShadowShort_factor * (((ShadowShort_avgPeriod != 0) ? (ShadowPeriodTotal / ShadowShort_avgPeriod) : ((ShadowShort_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((ShadowShort_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((ShadowShort_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)))) / ((ShadowShort_rangeType == 2) ? 2.0 : 1.0)))) && ((((double)inClose[i] >= (double)inOpen[i]) ? (double)inOpen[i] : (double)inClose[i]) - (double)inLow[i]) < ((ShadowShort_factor * (((ShadowShort_avgPeriod != 0) ? (ShadowPeriodTotal / ShadowShort_avgPeriod) : ((ShadowShort_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((ShadowShort_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((ShadowShort_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0)))) / ((ShadowShort_rangeType == 2) ? 2.0 : 1.0)))) ) {
            outInteger[outIdx++] = (((double)inClose[i] >= (double)inOpen[i]) ? 1 : 0 - 1) * 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         BodyPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0))) - ((BodyLong_rangeType == 0) ? (Math.abs((double)inClose[BodyTrailingIdx] - (double)inOpen[BodyTrailingIdx])) : ((BodyLong_rangeType == 1) ? ((double)inHigh[BodyTrailingIdx] - (double)inLow[BodyTrailingIdx]) : ((BodyLong_rangeType == 2) ? (((double)inHigh[BodyTrailingIdx] - (double)inLow[BodyTrailingIdx]) - Math.abs((double)inClose[BodyTrailingIdx] - (double)inOpen[BodyTrailingIdx])) : 0.0)));
         ShadowPeriodTotal += ((ShadowShort_rangeType == 0) ? (Math.abs((double)inClose[i] - (double)inOpen[i])) : ((ShadowShort_rangeType == 1) ? ((double)inHigh[i] - (double)inLow[i]) : ((ShadowShort_rangeType == 2) ? (((double)inHigh[i] - (double)inLow[i]) - Math.abs((double)inClose[i] - (double)inOpen[i])) : 0.0))) - ((ShadowShort_rangeType == 0) ? (Math.abs((double)inClose[ShadowTrailingIdx] - (double)inOpen[ShadowTrailingIdx])) : ((ShadowShort_rangeType == 1) ? ((double)inHigh[ShadowTrailingIdx] - (double)inLow[ShadowTrailingIdx]) : ((ShadowShort_rangeType == 2) ? (((double)inHigh[ShadowTrailingIdx] - (double)inLow[ShadowTrailingIdx]) - Math.abs((double)inClose[ShadowTrailingIdx] - (double)inOpen[ShadowTrailingIdx])) : 0.0)));
         i += 1;
         BodyTrailingIdx += 1;
         ShadowTrailingIdx += 1;
      } while( i <= endIdx );
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   /**
    * A single-candle pattern: a long real body with short upper and short lower
    * shadow. The signal direction follows the candle color (bullish if white,
    * bearish if black). Signals strong directional conviction on the bar: +100
    * white/bullish, -100 black/bearish. Not intrinsically a reversal or
    * continuation signal.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#CDLLONGLINE_Lookback} is a <b>success
    * with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inOpen Open price of each bar.
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param outInteger +100 on a white (close&gt;=open) long line, -100 on a
    *        black long line, 0 when no pattern. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#MAX_INDEX}, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, or two outputs share one array.
    * @throws NullPointerException if any input or output array is null.
    *
    * @see Core#CDLSHORTLINE
    * @see Core#CDLCLOSINGMARUBOZU
    * @see Core#CDLMARUBOZU
    * @see Core#CDLLONGLEGGEDDOJI
    */
   public OutRange CDLLONGLINE( int startIdx,
                                int endIdx,
                                double inOpen[],
                                double inHigh[],
                                double inLow[],
                                double inClose[],
                                int outInteger[] )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = CDLLONGLINE_Internal(startIdx, endIdx, inOpen, inHigh, inLow, inClose, outBegIdx, outNBElement, outInteger);
      if( retCode != RetCode.Success ) {
         throw failure("CDLLONGLINE", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * A single-candle pattern: a long real body with short upper and short lower
    * shadow. The signal direction follows the candle color (bullish if white,
    * bearish if black). Signals strong directional conviction on the bar: +100
    * white/bullish, -100 black/bearish. Not intrinsically a reversal or
    * continuation signal.
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#CDLLONGLINE_Lookback} is a <b>success
    * with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inOpen Open price of each bar.
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Close price of each bar.
    * @param outInteger +100 on a white (close&gt;=open) long line, -100 on a
    *        black long line, 0 when no pattern. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#MAX_INDEX}, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, or two outputs share one array.
    * @throws NullPointerException if any input or output array is null.
    *
    * @see Core#CDLSHORTLINE
    * @see Core#CDLCLOSINGMARUBOZU
    * @see Core#CDLMARUBOZU
    * @see Core#CDLLONGLEGGEDDOJI
    */
   public OutRange CDLLONGLINE( int startIdx,
                                int endIdx,
                                float inOpen[],
                                float inHigh[],
                                float inLow[],
                                float inClose[],
                                int outInteger[] )
   {
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = CDLLONGLINE_Internal(startIdx, endIdx, inOpen, inHigh, inLow, inClose, outBegIdx, outNBElement, outInteger);
      if( retCode != RetCode.Success ) {
         throw failure("CDLLONGLINE", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live CDLLONGLINE stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#CDLLONGLINE} over the same series.
    * Open with {@link Core#CDLLONGLINE_Open}; there is no close — the handle is
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
   public static final class CDLLONGLINE_Stream {
      final Core core;
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
      int cs_BodyLong_rangeType;
      int cs_BodyLong_avgPeriod;
      double cs_BodyLong_factor;
      int cs_ShadowShort_rangeType;
      int cs_ShadowShort_avgPeriod;
      double cs_ShadowShort_factor;
      int cur_outInteger;
      OutRange fillRange = OutRange.EMPTY;

      CDLLONGLINE_Stream( Core core ) { this.core = core; }

      /**
       * The range filled by {@link Core#CDLLONGLINE_OpenAndFill}, or
       * {@link OutRange#EMPTY} when this handle came from a plain
       * {@code open} (which fills nothing). Never {@code null}; a
       * successful {@code openAndFill} always writes at least one value,
       * so {@link OutRange#isEmpty()} tells the two apart.
       */
      public OutRange fillRange() { return fillRange; }

      CDLLONGLINE_Stream( CDLLONGLINE_Stream other ) {
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
         this.cs_BodyLong_rangeType = other.cs_BodyLong_rangeType;
         this.cs_BodyLong_avgPeriod = other.cs_BodyLong_avgPeriod;
         this.cs_BodyLong_factor = other.cs_BodyLong_factor;
         this.cs_ShadowShort_rangeType = other.cs_ShadowShort_rangeType;
         this.cs_ShadowShort_avgPeriod = other.cs_ShadowShort_avgPeriod;
         this.cs_ShadowShort_factor = other.cs_ShadowShort_factor;
         this.cur_outInteger = other.cur_outInteger;
         this.fillRange = other.fillRange;
      }

      /**
       * Commit one closed bar; always produces the new current value.
       * Never throws after a successful open; never allocates handle state.
       */
      public int update( double inOpen, double inHigh, double inLow, double inClose ) {
         core.CDLLONGLINE_StreamStep(this, inOpen, inHigh, inLow, inClose);
         return this.cur_outInteger;
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return (it is the same
       * generated code, run on a throwaway copy). Deep-copies the handle state
       * on every call: O(period) for windowed indicators — for hot loops,
       * prefer {@code update} on a {@code copy()}.
       */
      public int peek( double inOpen, double inHigh, double inLow, double inClose ) {
         CDLLONGLINE_Stream scratch = new CDLLONGLINE_Stream(this);
         core.CDLLONGLINE_StreamStep(scratch, inOpen, inHigh, inLow, inClose);
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
      public CDLLONGLINE_Stream copy() {
         return new CDLLONGLINE_Stream(this);
      }
   }
   void CDLLONGLINE_StreamStep( CDLLONGLINE_Stream sp, double inOpen, double inHigh, double inLow, double inClose )
   {
      int BodyLong_rangeType = sp.cs_BodyLong_rangeType;
      int BodyLong_avgPeriod = sp.cs_BodyLong_avgPeriod;
      double BodyLong_factor = sp.cs_BodyLong_factor;
      int ShadowShort_rangeType = sp.cs_ShadowShort_rangeType;
      int ShadowShort_avgPeriod = sp.cs_ShadowShort_avgPeriod;
      double ShadowShort_factor = sp.cs_ShadowShort_factor;
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
      if( Math.abs(inClose - inOpen) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (sp.BodyPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((BodyLong_rangeType == 1) ? (inHigh - inLow) : ((BodyLong_rangeType == 2) ? ((inHigh - inLow) - Math.abs(inClose - inOpen)) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && (inHigh - ((inClose >= inOpen) ? inClose : inOpen)) < ((ShadowShort_factor * (((ShadowShort_avgPeriod != 0) ? (sp.ShadowPeriodTotal / ShadowShort_avgPeriod) : ((ShadowShort_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((ShadowShort_rangeType == 1) ? (inHigh - inLow) : ((ShadowShort_rangeType == 2) ? ((inHigh - inLow) - Math.abs(inClose - inOpen)) : 0.0)))) / ((ShadowShort_rangeType == 2) ? 2.0 : 1.0)))) && (((inClose >= inOpen) ? inOpen : inClose) - inLow) < ((ShadowShort_factor * (((ShadowShort_avgPeriod != 0) ? (sp.ShadowPeriodTotal / ShadowShort_avgPeriod) : ((ShadowShort_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((ShadowShort_rangeType == 1) ? (inHigh - inLow) : ((ShadowShort_rangeType == 2) ? ((inHigh - inLow) - Math.abs(inClose - inOpen)) : 0.0)))) / ((ShadowShort_rangeType == 2) ? 2.0 : 1.0)))) ) {
         sp.cur_outInteger = ((inClose >= inOpen) ? 1 : 0 - 1) * 100;
      } else {
         sp.cur_outInteger = 0;
      }
      /* add the current range and subtract the first range: this is done after the pattern recognition
       * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
       */
      sp.BodyPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((BodyLong_rangeType == 1) ? (inHigh - inLow) : ((BodyLong_rangeType == 2) ? ((inHigh - inLow) - Math.abs(inClose - inOpen)) : 0.0))) - ((BodyLong_rangeType == 0) ? (Math.abs(sp.ring_BodyTrailingIdx_inClose[sp.ringPos_BodyTrailingIdx] - sp.ring_BodyTrailingIdx_inOpen[sp.ringPos_BodyTrailingIdx])) : ((BodyLong_rangeType == 1) ? (sp.ring_BodyTrailingIdx_inHigh[sp.ringPos_BodyTrailingIdx] - sp.ring_BodyTrailingIdx_inLow[sp.ringPos_BodyTrailingIdx]) : ((BodyLong_rangeType == 2) ? ((sp.ring_BodyTrailingIdx_inHigh[sp.ringPos_BodyTrailingIdx] - sp.ring_BodyTrailingIdx_inLow[sp.ringPos_BodyTrailingIdx]) - Math.abs(sp.ring_BodyTrailingIdx_inClose[sp.ringPos_BodyTrailingIdx] - sp.ring_BodyTrailingIdx_inOpen[sp.ringPos_BodyTrailingIdx])) : 0.0)));
      sp.ShadowPeriodTotal += ((ShadowShort_rangeType == 0) ? (Math.abs(inClose - inOpen)) : ((ShadowShort_rangeType == 1) ? (inHigh - inLow) : ((ShadowShort_rangeType == 2) ? ((inHigh - inLow) - Math.abs(inClose - inOpen)) : 0.0))) - ((ShadowShort_rangeType == 0) ? (Math.abs(sp.ring_ShadowTrailingIdx_inClose[sp.ringPos_ShadowTrailingIdx] - sp.ring_ShadowTrailingIdx_inOpen[sp.ringPos_ShadowTrailingIdx])) : ((ShadowShort_rangeType == 1) ? (sp.ring_ShadowTrailingIdx_inHigh[sp.ringPos_ShadowTrailingIdx] - sp.ring_ShadowTrailingIdx_inLow[sp.ringPos_ShadowTrailingIdx]) : ((ShadowShort_rangeType == 2) ? ((sp.ring_ShadowTrailingIdx_inHigh[sp.ringPos_ShadowTrailingIdx] - sp.ring_ShadowTrailingIdx_inLow[sp.ringPos_ShadowTrailingIdx]) - Math.abs(sp.ring_ShadowTrailingIdx_inClose[sp.ringPos_ShadowTrailingIdx] - sp.ring_ShadowTrailingIdx_inOpen[sp.ringPos_ShadowTrailingIdx])) : 0.0)));
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
   private RetCode CDLLONGLINE_OpenBody( CDLLONGLINE_Stream sp, double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx )
   {
      double BodyPeriodTotal = 0;
      double ShadowPeriodTotal = 0;
      int i = 0;
      int outIdx = 0;
      int BodyTrailingIdx = 0;
      int ShadowTrailingIdx = 0;
      int lookbackTotal = 0;
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      int lastValue_outInteger = 0;
      int historyLen = inOpen.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 || inHigh.length != inOpen.length || inLow.length != inOpen.length || inClose.length != inOpen.length ) {
         return RetCode.BadParam;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      int BodyLong_rangeType = this.candleSettings[CandleSettingType.BodyLong.ordinal()].rangeType.ordinal();
      int BodyLong_avgPeriod = this.candleSettings[CandleSettingType.BodyLong.ordinal()].avgPeriod;
      double BodyLong_factor = this.candleSettings[CandleSettingType.BodyLong.ordinal()].factor;
      int ShadowShort_rangeType = this.candleSettings[CandleSettingType.ShadowShort.ordinal()].rangeType.ordinal();
      int ShadowShort_avgPeriod = this.candleSettings[CandleSettingType.ShadowShort.ordinal()].avgPeriod;
      double ShadowShort_factor = this.candleSettings[CandleSettingType.ShadowShort.ordinal()].factor;
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = CDLLONGLINE_Lookback();
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
      BodyTrailingIdx = startIdx - BodyLong_avgPeriod;
      ShadowPeriodTotal = 0;
      ShadowTrailingIdx = startIdx - ShadowShort_avgPeriod;
      i = BodyTrailingIdx;
      while( i < startIdx ) {
         BodyPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyLong_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)));
         i += 1;
      }
      i = ShadowTrailingIdx;
      while( i < startIdx ) {
         ShadowPeriodTotal += ((ShadowShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowShort_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)));
         i += 1;
      }
      /* Proceed with the calculation for the requested range.
       * Must have:
       * - long real body
       * - short upper and lower shadow
       * The meaning of "long" and "short" is specified with TA_SetCandleSettings
       * outInteger is positive (1 to 100) when white (bullish), negative (-1 to -100) when black (bearish)
       */
      outIdx = 0;
      do {
         if( Math.abs(inClose[i] - inOpen[i]) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (BodyPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyLong_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && (inHigh[i] - ((inClose[i] >= inOpen[i]) ? inClose[i] : inOpen[i])) < ((ShadowShort_factor * (((ShadowShort_avgPeriod != 0) ? (ShadowPeriodTotal / ShadowShort_avgPeriod) : ((ShadowShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowShort_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)))) / ((ShadowShort_rangeType == 2) ? 2.0 : 1.0)))) && (((inClose[i] >= inOpen[i]) ? inOpen[i] : inClose[i]) - inLow[i]) < ((ShadowShort_factor * (((ShadowShort_avgPeriod != 0) ? (ShadowPeriodTotal / ShadowShort_avgPeriod) : ((ShadowShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowShort_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)))) / ((ShadowShort_rangeType == 2) ? 2.0 : 1.0)))) ) {
            lastValue_outInteger = ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) * 100;
         } else {
            lastValue_outInteger = 0;
         }
         /* add the current range and subtract the first range: this is done after the pattern recognition
          * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
          */
         BodyPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyLong_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0))) - ((BodyLong_rangeType == 0) ? (Math.abs(inClose[BodyTrailingIdx] - inOpen[BodyTrailingIdx])) : ((BodyLong_rangeType == 1) ? (inHigh[BodyTrailingIdx] - inLow[BodyTrailingIdx]) : ((BodyLong_rangeType == 2) ? ((inHigh[BodyTrailingIdx] - inLow[BodyTrailingIdx]) - Math.abs(inClose[BodyTrailingIdx] - inOpen[BodyTrailingIdx])) : 0.0)));
         ShadowPeriodTotal += ((ShadowShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowShort_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0))) - ((ShadowShort_rangeType == 0) ? (Math.abs(inClose[ShadowTrailingIdx] - inOpen[ShadowTrailingIdx])) : ((ShadowShort_rangeType == 1) ? (inHigh[ShadowTrailingIdx] - inLow[ShadowTrailingIdx]) : ((ShadowShort_rangeType == 2) ? ((inHigh[ShadowTrailingIdx] - inLow[ShadowTrailingIdx]) - Math.abs(inClose[ShadowTrailingIdx] - inOpen[ShadowTrailingIdx])) : 0.0)));
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
      sp.cs_BodyLong_rangeType = BodyLong_rangeType;
      sp.cs_BodyLong_avgPeriod = BodyLong_avgPeriod;
      sp.cs_BodyLong_factor = BodyLong_factor;
      sp.cs_ShadowShort_rangeType = ShadowShort_rangeType;
      sp.cs_ShadowShort_avgPeriod = ShadowShort_avgPeriod;
      sp.cs_ShadowShort_factor = ShadowShort_factor;
      sp.cur_outInteger = lastValue_outInteger;
      return RetCode.Success;
   }
   private RetCode CDLLONGLINE_OpenAndFillBody( CDLLONGLINE_Stream sp, double inOpen[], double inHigh[], double inLow[], double inClose[], MInteger outBegIdx, MInteger outNBElement, int outInteger[] )
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
      int startIdx = 0;
      if( historyLen < 1 || inHigh.length != inOpen.length || inLow.length != inOpen.length || inClose.length != inOpen.length ) {
         return RetCode.BadParam;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      if( (Object)outInteger == (Object)inOpen || (Object)outInteger == (Object)inHigh || (Object)outInteger == (Object)inLow || (Object)outInteger == (Object)inClose ) {
         return RetCode.BadParam;
      }
      int BodyLong_rangeType = this.candleSettings[CandleSettingType.BodyLong.ordinal()].rangeType.ordinal();
      int BodyLong_avgPeriod = this.candleSettings[CandleSettingType.BodyLong.ordinal()].avgPeriod;
      double BodyLong_factor = this.candleSettings[CandleSettingType.BodyLong.ordinal()].factor;
      int ShadowShort_rangeType = this.candleSettings[CandleSettingType.ShadowShort.ordinal()].rangeType.ordinal();
      int ShadowShort_avgPeriod = this.candleSettings[CandleSettingType.ShadowShort.ordinal()].avgPeriod;
      double ShadowShort_factor = this.candleSettings[CandleSettingType.ShadowShort.ordinal()].factor;
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = CDLLONGLINE_Lookback();
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
      BodyTrailingIdx = startIdx - BodyLong_avgPeriod;
      ShadowPeriodTotal = 0;
      ShadowTrailingIdx = startIdx - ShadowShort_avgPeriod;
      i = BodyTrailingIdx;
      while( i < startIdx ) {
         BodyPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyLong_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)));
         i += 1;
      }
      i = ShadowTrailingIdx;
      while( i < startIdx ) {
         ShadowPeriodTotal += ((ShadowShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowShort_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)));
         i += 1;
      }
      /* Proceed with the calculation for the requested range.
       * Must have:
       * - long real body
       * - short upper and lower shadow
       * The meaning of "long" and "short" is specified with TA_SetCandleSettings
       * outInteger is positive (1 to 100) when white (bullish), negative (-1 to -100) when black (bearish)
       */
      outIdx = 0;
      do {
         if( Math.abs(inClose[i] - inOpen[i]) > ((BodyLong_factor * (((BodyLong_avgPeriod != 0) ? (BodyPeriodTotal / BodyLong_avgPeriod) : ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyLong_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)))) / ((BodyLong_rangeType == 2) ? 2.0 : 1.0)))) && (inHigh[i] - ((inClose[i] >= inOpen[i]) ? inClose[i] : inOpen[i])) < ((ShadowShort_factor * (((ShadowShort_avgPeriod != 0) ? (ShadowPeriodTotal / ShadowShort_avgPeriod) : ((ShadowShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowShort_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)))) / ((ShadowShort_rangeType == 2) ? 2.0 : 1.0)))) && (((inClose[i] >= inOpen[i]) ? inOpen[i] : inClose[i]) - inLow[i]) < ((ShadowShort_factor * (((ShadowShort_avgPeriod != 0) ? (ShadowPeriodTotal / ShadowShort_avgPeriod) : ((ShadowShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowShort_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0)))) / ((ShadowShort_rangeType == 2) ? 2.0 : 1.0)))) ) {
            outInteger[outIdx++] = ((inClose[i] >= inOpen[i]) ? 1 : 0 - 1) * 100;
         } else {
            outInteger[outIdx++] = 0;
         }
         /* add the current range and subtract the first range: this is done after the pattern recognition
          * when avgPeriod is not 0, that means "compare with the previous candles" (it excludes the current candle)
          */
         BodyPeriodTotal += ((BodyLong_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((BodyLong_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((BodyLong_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0))) - ((BodyLong_rangeType == 0) ? (Math.abs(inClose[BodyTrailingIdx] - inOpen[BodyTrailingIdx])) : ((BodyLong_rangeType == 1) ? (inHigh[BodyTrailingIdx] - inLow[BodyTrailingIdx]) : ((BodyLong_rangeType == 2) ? ((inHigh[BodyTrailingIdx] - inLow[BodyTrailingIdx]) - Math.abs(inClose[BodyTrailingIdx] - inOpen[BodyTrailingIdx])) : 0.0)));
         ShadowPeriodTotal += ((ShadowShort_rangeType == 0) ? (Math.abs(inClose[i] - inOpen[i])) : ((ShadowShort_rangeType == 1) ? (inHigh[i] - inLow[i]) : ((ShadowShort_rangeType == 2) ? ((inHigh[i] - inLow[i]) - Math.abs(inClose[i] - inOpen[i])) : 0.0))) - ((ShadowShort_rangeType == 0) ? (Math.abs(inClose[ShadowTrailingIdx] - inOpen[ShadowTrailingIdx])) : ((ShadowShort_rangeType == 1) ? (inHigh[ShadowTrailingIdx] - inLow[ShadowTrailingIdx]) : ((ShadowShort_rangeType == 2) ? ((inHigh[ShadowTrailingIdx] - inLow[ShadowTrailingIdx]) - Math.abs(inClose[ShadowTrailingIdx] - inOpen[ShadowTrailingIdx])) : 0.0)));
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
      sp.cs_BodyLong_rangeType = BodyLong_rangeType;
      sp.cs_BodyLong_avgPeriod = BodyLong_avgPeriod;
      sp.cs_BodyLong_factor = BodyLong_factor;
      sp.cs_ShadowShort_rangeType = ShadowShort_rangeType;
      sp.cs_ShadowShort_avgPeriod = ShadowShort_avgPeriod;
      sp.cs_ShadowShort_factor = ShadowShort_factor;
      sp.cur_outInteger = outInteger[outNBElement.value - 1];
      return RetCode.Success;
   }
   /* Internal startIdx-anchored open behind CDLLONGLINE_Open (composition seam). */
   CDLLONGLINE_Stream CDLLONGLINE_OpenInternal( double inOpen[], double inHigh[], double inLow[], double inClose[], int startIdx )
   {
      CDLLONGLINE_Stream sp = new CDLLONGLINE_Stream(this);
      RetCode retCode = CDLLONGLINE_OpenBody(sp, inOpen, inHigh, inLow, inClose, startIdx);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("CDLLONGLINE open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("CDLLONGLINE open: internal error");
      }
      throw new IllegalArgumentException("CDLLONGLINE open: " + retCode);
   }
   /**
    * Open a live CDLLONGLINE stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#CDLLONGLINE} at that bar.
    * <p>The history must hold at least {@code CDLLONGLINE_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public CDLLONGLINE_Stream CDLLONGLINE_Open( double inOpen[], double inHigh[], double inLow[], double inClose[] )
   {
      return CDLLONGLINE_OpenInternal(inOpen, inHigh, inLow, inClose, 0);
   }
   /**
    * {@link Core#CDLLONGLINE_Open} that also fills the output array(s) bit-identically
    * to {@link Core#CDLLONGLINE} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    * <p>The range written is on the returned handle:
    * {@link CDLLONGLINE_Stream#fillRange()}.
    */
   public CDLLONGLINE_Stream CDLLONGLINE_OpenAndFill( double inOpen[], double inHigh[], double inLow[], double inClose[], int outInteger[] )
   {
      CDLLONGLINE_Stream sp = new CDLLONGLINE_Stream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = CDLLONGLINE_OpenAndFillBody(sp, inOpen, inHigh, inLow, inClose, outBegIdx, outNBElement, outInteger);
      sp.fillRange = new OutRange(outBegIdx.value, outNBElement.value);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("CDLLONGLINE openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("CDLLONGLINE openAndFill: internal error");
      }
      throw new IllegalArgumentException("CDLLONGLINE openAndFill: " + retCode);
   }
