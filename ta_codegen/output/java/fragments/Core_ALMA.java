/* Arnaud Legoux Moving Average, created by Arnaud Legoux and Dimitris
 * Kouzis-Loukas: "ALMA; In search for the perfect Moving Average", 2009,
 * https://web.archive.org/web/20110904091012/www.arnaudlegoux.com/wp-content/uploads/2011/03/ALMA-Arnaud-Legoux-Moving-Average.pdf
 *
 * List of contributors:
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
 *  092926 MF,CC  First version (issue #475).
 */

/* Using alma_ALT1 for TA_ALT={BATCH,ALL_LANGUAGES} */

   /**
    * Number of leading input bars {@link Core#alma} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInTimePeriod Number of bars in the window (default 9; range
    *        1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInSigma Divides the period to give the Gaussian's width in bars
    *        (default 6; minimum 0.01; {@link Core#REAL_DEFAULT} selects the default).
    * @param optInOffset Position of the peak weight, 0 at the oldest bar and 1
    *        at the newest (default 0.85; range 0..1; {@link Core#REAL_DEFAULT} selects
    *        the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int almaLookback( int optInTimePeriod, double optInSigma, double optInOffset )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 9;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return -1;
      }
      if( optInSigma == REAL_DEFAULT ) {
         optInSigma = 6e0;
      } else if( !(optInSigma >= 1e-2 && optInSigma <= REAL_MAX) ) {
         return -1;
      }
      if( optInOffset == REAL_DEFAULT ) {
         optInOffset = 8.5e-1;
      } else if( !(optInOffset >= 0e0 && optInOffset <= 1e0) ) {
         return -1;
      }
      return optInTimePeriod - 1 ;

   }
   /**
    * How many bars ahead (positive) or behind (negative) of the bar that
    * computed it a chart draws one output of {@link Core#alma}.
    * <p>Every output of this function is drawn at its own bar, so the answer is
    * 0.
    *
    * @param optInTimePeriod Number of bars in the window (default 9; range
    *        1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInSigma Divides the period to give the Gaussian's width in bars
    *        (default 6; minimum 0.01; {@link Core#REAL_DEFAULT} selects the default).
    * @param optInOffset Position of the peak weight, 0 at the oldest bar and 1
    *        at the newest (default 0.85; range 0..1; {@link Core#REAL_DEFAULT} selects
    *        the default).
    * @param outputIdx Position of the output in the batch signature, from 0.
    * @return The display shift, or {@code Integer.MIN_VALUE} if a parameter is
    *        out of range or the index names no output.
    */
   public int almaDisplayShift( int optInTimePeriod, double optInSigma, double optInOffset, int outputIdx )
   {
      if( almaLookback( optInTimePeriod, optInSigma, optInOffset ) < 0 ) {
         return Integer.MIN_VALUE;
      }
      if( outputIdx < 0 || outputIdx >= 1 ) {
         return Integer.MIN_VALUE;
      }
      return 0;
   }
   RetCode almaImpl( int startIdx,
                     int endIdx,
                     double inReal[],
                     int optInTimePeriod,
                     double optInSigma,
                     double optInOffset,
                     MInteger outBegIdx,
                     MInteger outNBElement,
                     double outReal[] )
   {
      double sum = 0;
      double s = 0;
      double twoSSq = 0;
      double d = 0;
      double norm = 0;
      double wj = 0;
      double s0 = 0;
      double s1 = 0;
      double s2 = 0;
      double s3 = 0;
      int lookbackTotal = 0;
      int outIdx = 0;
      int i = 0;
      int j = 0;
      int w = 0;
      int m = 0;
      int b = 0;
      double[] weights;
      int weights_Idx = 0;
      int maxIdx_weights = (30)-1;
      if( (startIdx < 0) || (startIdx > INDEX_MAX) ) {
         return RetCode.OUT_OF_RANGE_START_INDEX ;
      }
      if( (endIdx < 0) || (endIdx > INDEX_MAX) || (endIdx < startIdx)) {
         return RetCode.OUT_OF_RANGE_END_INDEX ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 9;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInSigma == REAL_DEFAULT ) {
         optInSigma = 6e0;
      } else if( !(optInSigma >= 1e-2 && optInSigma <= REAL_MAX) ) {
         return RetCode.BAD_PARAM;
      }
      if( optInOffset == REAL_DEFAULT ) {
         optInOffset = 8.5e-1;
      } else if( !(optInOffset >= 0e0 && optInOffset <= 1e0) ) {
         return RetCode.BAD_PARAM;
      }
      /* Filled once and read as a plain array. */
      lookbackTotal = optInTimePeriod - 1;
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.SUCCESS ;
      }
      /* A dot product seeded with 0.0 would turn a -0.0 input into +0.0. */
      if( optInTimePeriod == 1 ) {
         outBegIdx.value = startIdx;
         outNBElement.value = endIdx - startIdx + 1;
         i = startIdx;
         for( outIdx = 0; outIdx < (int)outNBElement.value; outIdx += 1 ) {
            outReal[outIdx] = inReal[i++];
         }
         return RetCode.SUCCESS ;
      }
      if( optInTimePeriod < 1 ) return RetCode.INTERNAL_ERROR;
      weights = new double[optInTimePeriod];
      maxIdx_weights = (optInTimePeriod)-1;
      weights_Idx = 0;
      /* The expression order is the spec: it is what makes the output bitwise
       * equal to Tulip's beta/alma.c. Keep the floor in binary64 and (j-m) squared
       * in double (an int square overflows at large periods).
       */
      m = (int)Math.floor(optInOffset * (double)lookbackTotal);
      s = (double)optInTimePeriod / optInSigma;
      twoSSq = 2.0 * s * s;
      norm = 0.0;
      for( j = 0; j < optInTimePeriod; j += 1 ) {
         d = (double)(j - m);
         weights[j] = Math.exp(-(d * d) / twoSSq);
         norm += weights[j];
      }
      for( j = 0; j < optInTimePeriod; j += 1 ) {
         weights[j] = weights[j] / norm;
      }
      /* 4 bars per pass, each with its own accumulator summed in the same
       * order as the base: the same bits, with 4 independent add chains in
       * flight instead of one. Every window of a pass is read before any of its
       * outputs is written, and the next pass reads past them, so outReal may
       * alias inReal.
       */
      outIdx = 0;
      i = startIdx;
      while( i + 3 <= endIdx ) {
         s0 = 0.0;
         s1 = 0.0;
         s2 = 0.0;
         s3 = 0.0;
         b = i - lookbackTotal;
         for( j = 0; j < optInTimePeriod; j += 1 ) {
            wj = weights[j];
            s0 += wj * inReal[b];
            s1 += wj * inReal[b + 1];
            s2 += wj * inReal[b + 2];
            s3 += wj * inReal[b + 3];
            b += 1;
         }
         outReal[outIdx] = s0;
         outReal[outIdx + 1] = s1;
         outReal[outIdx + 2] = s2;
         outReal[outIdx + 3] = s3;
         outIdx += 4;
         i += 4;
      }
      while( i <= endIdx ) {
         sum = 0.0;
         w = 0;
         for( j = i - lookbackTotal; j <= i; j += 1 ) {
            sum += weights[w] * inReal[j];
            w += 1;
         }
         outReal[outIdx] = sum;
         outIdx += 1;
         i += 1;
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.SUCCESS ;
   }
   RetCode almaImpl( int startIdx,
                     int endIdx,
                     float inReal[],
                     int optInTimePeriod,
                     double optInSigma,
                     double optInOffset,
                     MInteger outBegIdx,
                     MInteger outNBElement,
                     double outReal[] )
   {
      double sum = 0;
      double s = 0;
      double twoSSq = 0;
      double d = 0;
      double norm = 0;
      double wj = 0;
      double s0 = 0;
      double s1 = 0;
      double s2 = 0;
      double s3 = 0;
      int lookbackTotal = 0;
      int outIdx = 0;
      int i = 0;
      int j = 0;
      int w = 0;
      int m = 0;
      int b = 0;
      double[] weights;
      int weights_Idx = 0;
      int maxIdx_weights = (30)-1;
      if( (startIdx < 0) || (startIdx > INDEX_MAX) ) {
         return RetCode.OUT_OF_RANGE_START_INDEX ;
      }
      if( (endIdx < 0) || (endIdx > INDEX_MAX) || (endIdx < startIdx)) {
         return RetCode.OUT_OF_RANGE_END_INDEX ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 9;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInSigma == REAL_DEFAULT ) {
         optInSigma = 6e0;
      } else if( !(optInSigma >= 1e-2 && optInSigma <= REAL_MAX) ) {
         return RetCode.BAD_PARAM;
      }
      if( optInOffset == REAL_DEFAULT ) {
         optInOffset = 8.5e-1;
      } else if( !(optInOffset >= 0e0 && optInOffset <= 1e0) ) {
         return RetCode.BAD_PARAM;
      }
      lookbackTotal = optInTimePeriod - 1;
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.SUCCESS ;
      }
      if( optInTimePeriod == 1 ) {
         outBegIdx.value = startIdx;
         outNBElement.value = endIdx - startIdx + 1;
         i = startIdx;
         for( outIdx = 0; outIdx < (int)outNBElement.value; outIdx += 1 ) {
            outReal[outIdx] = (double)inReal[i++];
         }
         return RetCode.SUCCESS ;
      }
      if( optInTimePeriod < 1 ) return RetCode.INTERNAL_ERROR;
      weights = new double[optInTimePeriod];
      maxIdx_weights = (optInTimePeriod)-1;
      weights_Idx = 0;
      m = (int)Math.floor(optInOffset * (double)lookbackTotal);
      s = (double)optInTimePeriod / optInSigma;
      twoSSq = 2.0 * s * s;
      norm = 0.0;
      for( j = 0; j < optInTimePeriod; j += 1 ) {
         d = (double)(j - m);
         weights[j] = Math.exp(-(d * d) / twoSSq);
         norm += weights[j];
      }
      for( j = 0; j < optInTimePeriod; j += 1 ) {
         weights[j] = weights[j] / norm;
      }
      outIdx = 0;
      i = startIdx;
      while( i + 3 <= endIdx ) {
         s0 = 0.0;
         s1 = 0.0;
         s2 = 0.0;
         s3 = 0.0;
         b = i - lookbackTotal;
         for( j = 0; j < optInTimePeriod; j += 1 ) {
            wj = weights[j];
            s0 += wj * (double)inReal[b];
            s1 += wj * (double)inReal[b + 1];
            s2 += wj * (double)inReal[b + 2];
            s3 += wj * (double)inReal[b + 3];
            b += 1;
         }
         outReal[outIdx] = s0;
         outReal[outIdx + 1] = s1;
         outReal[outIdx + 2] = s2;
         outReal[outIdx + 3] = s3;
         outIdx += 4;
         i += 4;
      }
      while( i <= endIdx ) {
         sum = 0.0;
         w = 0;
         for( j = i - lookbackTotal; j <= i; j += 1 ) {
            sum += weights[w] * (double)inReal[j];
            w += 1;
         }
         outReal[outIdx] = sum;
         outIdx += 1;
         i += 1;
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.SUCCESS ;
   }
   /**
    * Arnaud Legoux Moving Average: the last N inputs weighted by a Gaussian
    * whose peak sits a fraction {@code offset} of the way from the oldest to
    * the newest bar. Moving the offset toward 1 puts the peak on recent bars
    * and cuts the lag; moving it toward 0 puts the peak on old bars and lags
    * most. Smoothing is greatest with the peak mid-window, near 0.5. Sigma sets
    * the width: the Gaussian's standard deviation is {@code N / sigma} bars, so
    * a larger sigma concentrates the weight around the peak and smooths less.
    * The weights are non-negative and sum to one, so the line stays within the
    * range of its window, up to rounding. At the default shape and a period of
    * 5 or more, it lags about half as much as an {@code SMA} of the same period
    * and passes about twice its noise.
    * <p>Formula and more info at <a
    * href="https://ta-lib.org/functions/alma">ta-lib.org/functions/alma</a>.
    * <p><b>Notes</b>
    * <ul>
    * <li>The peak is floored to a whole bar, as in the authors' code. At periods below about 5 this puts it on an older bar: at period 2 the line is almost entirely the previous bar.</li>
    * <li>{@code TA_MAType_ALMA} runs this function at the default sigma and offset, so the line in {@code MA} and every function taking an MAType has the lag and noise stated above.</li>
    * <li>The published paper's summary formula uses a different parameterisation; this is the form of the authors' own implementation.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are, and the
    * library never pads with NaN. A valid range that ends before
    * {@link Core#almaLookback} is a <b>success with no values</b>
    * ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Data on which to compute the average.
    * @param optInTimePeriod Number of bars in the window (default 9; range
    *        1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInSigma Divides the period to give the Gaussian's width in bars
    *        (default 6; minimum 0.01; {@link Core#REAL_DEFAULT} selects the default).
    * @param optInOffset Position of the peak weight, 0 at the oldest bar and 1
    *        at the newest (default 0.85; range 0..1; {@link Core#REAL_DEFAULT} selects
    *        the default).
    * @param outReal Arnaud Legoux Moving Average line. Must hold at least
    *        {@code endIdx - max(startIdx, almaLookback(...)) + 1} values, the count
    *        the call produces (none when that is not positive).
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#INDEX_MAX}, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, two outputs share one array, or an array is absent or
    *        too short for the range requested — any input this function
    *        <i>declares</i> that does not reach {@code endIdx}, or an output that
    *        cannot hold the values produced. Declared, not read: a few candlestick
    *        patterns take an OHLC series they never index, and it is required all the
    *        same. An output this function documents as declinable is the one
    *        exception: {@code null} is how you decline it. Checked before anything is
    *        written, so a rejected call leaves every buffer untouched.
    *
    * @see Core#wma
    * @see Core#sma
    * @see Core#trima
    * @see Core#hma
    */
   public OutRange alma( int startIdx,
                         int endIdx,
                         double inReal[],
                         int optInTimePeriod,
                         double optInSigma,
                         double optInOffset,
                         double outReal[] )
   {
      requireIndexRange("ALMA", startIdx, endIdx);
      int guardStart = clampedStart("ALMA", startIdx, almaLookback(optInTimePeriod, optInSigma, optInOffset));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("ALMA", "inReal", inReal, guardInLen);
      requireLength("ALMA", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = almaImpl(startIdx, endIdx, inReal, optInTimePeriod, optInSigma, optInOffset, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.SUCCESS ) {
         throw failure("ALMA", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Arnaud Legoux Moving Average: the last N inputs weighted by a Gaussian
    * whose peak sits a fraction {@code offset} of the way from the oldest to
    * the newest bar. Moving the offset toward 1 puts the peak on recent bars
    * and cuts the lag; moving it toward 0 puts the peak on old bars and lags
    * most. Smoothing is greatest with the peak mid-window, near 0.5. Sigma sets
    * the width: the Gaussian's standard deviation is {@code N / sigma} bars, so
    * a larger sigma concentrates the weight around the peak and smooths less.
    * The weights are non-negative and sum to one, so the line stays within the
    * range of its window, up to rounding. At the default shape and a period of
    * 5 or more, it lags about half as much as an {@code SMA} of the same period
    * and passes about twice its noise.
    * <p>Formula and more info at <a
    * href="https://ta-lib.org/functions/alma">ta-lib.org/functions/alma</a>.
    * <p><b>Notes</b>
    * <ul>
    * <li>The peak is floored to a whole bar, as in the authors' code. At periods below about 5 this puts it on an older bar: at period 2 the line is almost entirely the previous bar.</li>
    * <li>{@code TA_MAType_ALMA} runs this function at the default sigma and offset, so the line in {@code MA} and every function taking an MAType has the lag and noise stated above.</li>
    * <li>The published paper's summary formula uses a different parameterisation; this is the form of the authors' own implementation.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are, and the
    * library never pads with NaN. A valid range that ends before
    * {@link Core#almaLookback} is a <b>success with no values</b>
    * ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Data on which to compute the average.
    * @param optInTimePeriod Number of bars in the window (default 9; range
    *        1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInSigma Divides the period to give the Gaussian's width in bars
    *        (default 6; minimum 0.01; {@link Core#REAL_DEFAULT} selects the default).
    * @param optInOffset Position of the peak weight, 0 at the oldest bar and 1
    *        at the newest (default 0.85; range 0..1; {@link Core#REAL_DEFAULT} selects
    *        the default).
    * @param outReal Arnaud Legoux Moving Average line. Must hold at least
    *        {@code endIdx - max(startIdx, almaLookback(...)) + 1} values, the count
    *        the call produces (none when that is not positive).
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#INDEX_MAX}, or {@code endIdx < startIdx}.
    * @throws IllegalArgumentException if an optional parameter is outside its
    *        documented range, two outputs share one array, or an array is absent or
    *        too short for the range requested — any input this function
    *        <i>declares</i> that does not reach {@code endIdx}, or an output that
    *        cannot hold the values produced. Declared, not read: a few candlestick
    *        patterns take an OHLC series they never index, and it is required all the
    *        same. An output this function documents as declinable is the one
    *        exception: {@code null} is how you decline it. Checked before anything is
    *        written, so a rejected call leaves every buffer untouched.
    *
    * @see Core#wma
    * @see Core#sma
    * @see Core#trima
    * @see Core#hma
    */
   public OutRange alma( int startIdx,
                         int endIdx,
                         float inReal[],
                         int optInTimePeriod,
                         double optInSigma,
                         double optInOffset,
                         double outReal[] )
   {
      requireIndexRange("ALMA", startIdx, endIdx);
      int guardStart = clampedStart("ALMA", startIdx, almaLookback(optInTimePeriod, optInSigma, optInOffset));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("ALMA", "inReal", inReal, guardInLen);
      requireLength("ALMA", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = almaImpl(startIdx, endIdx, inReal, optInTimePeriod, optInSigma, optInOffset, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.SUCCESS ) {
         throw failure("ALMA", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live ALMA stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#alma} over the same series.
    * Open with {@link Core#almaOpen}; there is no close — the handle is
    * ordinary heap state, unreferenced handles are simply garbage-collected.
    * <p>Concurrency: a handle is single-writer — {@code update}, {@code peek},
    * {@code value} and {@code clone} must not race with an {@code update} on
    * the same handle. With no concurrent {@code update}, {@code peek}/
    * {@code value}/{@code clone} never write the stream and may be called
    * concurrently after safe publication. Independent streams (a
    * {@code clone()} result included) are fully independent.
    * <p>Not serializable by design: to checkpoint, retain the history and
    * re-open — the result is bit-identical by contract.
    */
   public static final class AlmaStream {
      private Core core;
      private int optInTimePeriod;
      private double optInSigma;
      private double optInOffset;
      private int lookbackTotal;
      private int weights_Idx;
      private int maxIdx_weights;
      private int winPos_j;
      private int winCap_j;
      private double[] win_j_inReal;
      private int cbSize_weights;
      private double[] cb_weights;
      private double cur_outReal;
      private int outRangeBegIdx;
      private int outRangeCount;

      private AlmaStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#alma} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * accepted {@code update} adds one to the count — a rejected one
       * changes nothing, and neither does {@code peek} — and
       * {@code clone()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       * <p>The last bar it can reach is {@link Core#INDEX_MAX}; past that
       * {@code update} and {@code advance} throw
       * {@link IndexOutOfBoundsException}.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      /**
       * Count one bar this stream was not fed: {@link #outRange()} advances
       * by one and nothing else moves — {@link #value()} keeps answering the previous
       * output, which is this bar's output too.
       * <p>For a bar the caller leaves out: one an {@code update} rejected
       * and that will not be re-fed, or a session with no print. Without it
       * two handles on one feed drift a bar apart when only one of them skips.
       * <p>Throws {@link IndexOutOfBoundsException} once {@link #outRange()}
       * has reached bar {@link Core#INDEX_MAX}, the last one the batch tier
       * can address and the last this handle will count. {@code update}
       * throws the same there.
       */
      public void advance() {
         if( this.outRangeBegIdx + this.outRangeCount > INDEX_MAX )
            throw failure("ALMA advance", RetCode.OUT_OF_RANGE_END_INDEX);
         this.outRangeCount++;
      }

      private AlmaStream( AlmaStream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.optInSigma = other.optInSigma;
         this.optInOffset = other.optInOffset;
         this.lookbackTotal = other.lookbackTotal;
         this.weights_Idx = other.weights_Idx;
         this.maxIdx_weights = other.maxIdx_weights;
         this.winPos_j = other.winPos_j;
         this.winCap_j = other.winCap_j;
         this.win_j_inReal = other.win_j_inReal.clone();
         this.cbSize_weights = other.cbSize_weights;
         this.cb_weights = other.cb_weights.clone();
         this.cur_outReal = other.cur_outReal;
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      /**
       * Commit one closed bar, returning the new current value.
       * <p>Throws {@link IllegalArgumentException} if any bar value is not
       * finite (NaN or an infinity). That check runs before anything is
       * written, so nothing moves — {@link #outRange()} included — and
       * {@link #value()} still answers the previous value. Re-feed the bar when a
       * corrected value arrives, or call {@link #advance()} to count it and
       * carry on; two handles on one feed drift a bar apart if neither
       * happens.
       * This is the one place the streaming tier is stricter than
       * the batch API, which computes on whatever it is given: a handle
       * retains its state, so a single non-finite bar would poison every
       * later value it produces.
       * <p>Throws {@link IndexOutOfBoundsException} once {@link #outRange()}
       * has reached bar {@link Core#INDEX_MAX}, which no re-feed clears: the
       * handle has run out of index domain and only a shorter history can
       * start a new one.
       */
      public double update( double inReal ) {
         if( this.outRangeBegIdx + this.outRangeCount > INDEX_MAX )
            throw failure("ALMA update", RetCode.OUT_OF_RANGE_END_INDEX);
         if( !Double.isFinite(inReal) )
            throw nonFiniteBar("ALMA update", "inReal");
         core.almaStepImpl(this, inReal);
         this.outRangeCount++;
         return this.cur_outReal;
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return — the same
       * transition, with every store it would make carried in a local instead.
       * Never writes this handle, so peeks may run concurrently with each other.
       * <p>It counts no bar, so it keeps answering past the
       * {@link Core#INDEX_MAX} ceiling {@code update} stops at.
       */
      public double peek( double inReal ) {
         if( !Double.isFinite(inReal) )
            throw nonFiniteBar("ALMA peek", "inReal");
         AlmaStream sp = this;
         double sum = 0.0;
         int j = 0;
         int w = 0;
         double cur_outReal = 0.0;
         int pkSlot0 = -1;
         double pkVal0 = 0.0;
         if( sp.optInTimePeriod == 1 ) {
            cur_outReal = inReal;
            return cur_outReal ;
         }
         pkSlot0 = sp.winPos_j;
         pkVal0 = inReal;
         sum = 0.0;
         w = 0;
         for( j = sp.lookbackTotal; j >= 0; j -= 1 ) {
            sum += sp.cb_weights[w] * ((((sp.winPos_j + sp.winCap_j - j >= sp.winCap_j) ? sp.winPos_j + sp.winCap_j - j - sp.winCap_j : sp.winPos_j + sp.winCap_j - j) != pkSlot0) ? sp.win_j_inReal[(sp.winPos_j + sp.winCap_j - j >= sp.winCap_j) ? sp.winPos_j + sp.winCap_j - j - sp.winCap_j : sp.winPos_j + sp.winCap_j - j] : pkVal0);
            w += 1;
         }
         cur_outReal = sum;
         return cur_outReal;
      }

      /**
       * The value at the last bar this stream counted — the bar
       * {@link #outRange()} ends on. The last history bar right after open,
       * then whatever the latest accepted {@code update} returned.
       * A pure field read; {@code peek} does not change it.
       */
      public double value() {
         return this.cur_outReal;
      }

      /**
       * An independent fork of this stream: both evolve separately from here
       * on. Buffers are copied and sub-streams cloned recursively; the
       * {@link Core} reference is shared, since a {@code Core} is immutable
       * for a stream's lifetime.
       *
       * <p>Not the {@code Cloneable} protocol: this calls a copy constructor,
       * never {@code super.clone()}, so it throws nothing.
       *
       * @return an independent stream at the same bar
       */
      @Override
      public AlmaStream clone() {
         return new AlmaStream(this);
      }
   }
   private void almaStepImpl( AlmaStream sp, double inReal )
   {
      double sum = 0.0;
      int j = 0;
      int w = 0;
      if( sp.optInTimePeriod == 1 ) {
         sp.cur_outReal = inReal;
         return ;
      }
      sp.win_j_inReal[sp.winPos_j] = inReal;
      sum = 0.0;
      w = 0;
      for( j = sp.lookbackTotal; j >= 0; j -= 1 ) {
         sum += sp.cb_weights[w] * sp.win_j_inReal[(sp.winPos_j + sp.winCap_j - j >= sp.winCap_j) ? sp.winPos_j + sp.winCap_j - j - sp.winCap_j : sp.winPos_j + sp.winCap_j - j];
         w += 1;
      }
      sp.cur_outReal = sum;
      sp.winPos_j = sp.winPos_j + 1;
      if( sp.winPos_j >= sp.winCap_j ) {
         sp.winPos_j = 0;
      }
   }
   private RetCode almaOpenImpl( AlmaStream sp, double inReal[], int startIdx, int optInTimePeriod, double optInSigma, double optInOffset, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
   {
      double sum = 0;
      double s = 0;
      double twoSSq = 0;
      double d = 0;
      double norm = 0;
      int lookbackTotal = 0;
      int outIdx = 0;
      int i = 0;
      int j = 0;
      int w = 0;
      int m = 0;
      double[] weights;
      int weights_Idx = 0;
      int maxIdx_weights = (30)-1;
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.OUT_OF_RANGE_START_INDEX;
      }
      if( historyLen > INDEX_MAX + 1 ) {
         return RetCode.OUT_OF_RANGE_END_INDEX;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 9;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInSigma == REAL_DEFAULT ) {
         optInSigma = 6e0;
      } else if( !(optInSigma >= 1e-2 && optInSigma <= REAL_MAX) ) {
         return RetCode.BAD_PARAM;
      }
      if( optInOffset == REAL_DEFAULT ) {
         optInOffset = 8.5e-1;
      } else if( !(optInOffset >= 0e0 && optInOffset <= 1e0) ) {
         return RetCode.BAD_PARAM;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.INSUFFICIENT_HISTORY;
      }
      if( optInTimePeriod == 1 ) {
         int fillLb = almaLookback(optInTimePeriod, optInSigma, optInOffset);
         if( startIdx > fillLb ) fillLb = startIdx;
         if( historyLen < fillLb + 1 ) {
            return RetCode.INSUFFICIENT_HISTORY;
         }
         sp.optInTimePeriod = optInTimePeriod;
         sp.optInSigma = optInSigma;
         sp.optInOffset = optInOffset;
         sp.lookbackTotal = 0;
         sp.weights_Idx = 0;
         sp.maxIdx_weights = 0;
         sp.winPos_j = 0;
         sp.winCap_j = 1;
         sp.win_j_inReal = new double[1];
         sp.cbSize_weights = 0;
         sp.cb_weights = new double[1];
         outBegIdx.value = fillLb;
         outNBElement.value = historyLen - fillLb;
         if( outStride == 0 ) {
            outReal[0] = inReal[historyLen - 1];
         } else {
            for( int fillIdx = 0; fillIdx < historyLen - fillLb; fillIdx++ ) {
               outReal[fillIdx] = inReal[fillLb + fillIdx];
            }
         }
         sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
         return RetCode.SUCCESS;
      }
      /* Filled once and read as a plain array. */
      lookbackTotal = optInTimePeriod - 1;
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.INSUFFICIENT_HISTORY ;
      }
      if( optInTimePeriod < 1 ) return RetCode.INTERNAL_ERROR;
      weights = new double[optInTimePeriod];
      maxIdx_weights = (optInTimePeriod)-1;
      weights_Idx = 0;
      /* The expression order is the spec: it is what makes the output bitwise
       * equal to Tulip's beta/alma.c. Keep the floor in binary64 and (j-m) squared
       * in double (an int square overflows at large periods).
       */
      m = (int)Math.floor(optInOffset * (double)lookbackTotal);
      s = (double)optInTimePeriod / optInSigma;
      twoSSq = 2.0 * s * s;
      norm = 0.0;
      for( j = 0; j < optInTimePeriod; j += 1 ) {
         d = (double)(j - m);
         weights[j] = Math.exp(-(d * d) / twoSSq);
         norm += weights[j];
      }
      for( j = 0; j < optInTimePeriod; j += 1 ) {
         weights[j] = weights[j] / norm;
      }
      /* Weight 0 is the oldest bar. Each output is written after its window is
       * read, and the next window starts past it, so outReal may alias inReal.
       */
      outIdx = 0;
      i = startIdx;
      while( i <= endIdx ) {
         sum = 0.0;
         w = 0;
         for( j = i - lookbackTotal; j <= i; j += 1 ) {
            sum += weights[w] * inReal[j];
            w += 1;
         }
         outReal[outIdx * outStride] = sum;
         outIdx += 1;
         i += 1;
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      /* Capture the live batch state into the handle. */
      int cap_j = (int)(lookbackTotal + 1);
      if( cap_j < 1 || cap_j > historyLen ) {
         return RetCode.INTERNAL_ERROR;
      }
      double[] capWin_j_inReal = new double[cap_j];
      System.arraycopy(inReal, historyLen - cap_j, capWin_j_inReal, 0, cap_j);
      int capCb_weights = maxIdx_weights + 1;
      if( capCb_weights > historyLen + 1 ) {
         return RetCode.INTERNAL_ERROR;
      }
      sp.optInTimePeriod = optInTimePeriod;
      sp.optInSigma = optInSigma;
      sp.optInOffset = optInOffset;
      sp.lookbackTotal = lookbackTotal;
      sp.weights_Idx = weights_Idx;
      sp.maxIdx_weights = maxIdx_weights;
      sp.winPos_j = 0;
      sp.winCap_j = cap_j;
      sp.win_j_inReal = capWin_j_inReal;
      sp.cbSize_weights = capCb_weights;
      sp.cb_weights = weights;
      sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
      return RetCode.SUCCESS;
   }
   /* almaOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   AlmaStream almaOpenAndFillInternal( double inReal[], int startIdx, int optInTimePeriod, double optInSigma, double optInOffset, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      AlmaStream sp = new AlmaStream(this);
      RetCode retCode = almaOpenImpl(sp, inReal, startIdx, optInTimePeriod, optInSigma, optInOffset, outBegIdx, outNBElement, outReal, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.SUCCESS ) {
         return sp;
      }
      if( retCode == RetCode.INSUFFICIENT_HISTORY ) {
         throw insufficientHistory("ALMA openAndFill", inReal.length, startIdx, almaLookback(optInTimePeriod, optInSigma, optInOffset));
      }
      throw streamFailure("ALMA openAndFill", retCode);
   }
   /* Internal startIdx-anchored open behind almaOpen (composition seam). */
   AlmaStream almaOpenInternal( double inReal[], int startIdx, int optInTimePeriod, double optInSigma, double optInOffset )
   {
      AlmaStream sp = new AlmaStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      RetCode retCode = almaOpenImpl(sp, inReal, startIdx, optInTimePeriod, optInSigma, optInOffset, outBegIdx, outNBElement, sink_outReal, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.SUCCESS ) {
         return sp;
      }
      if( retCode == RetCode.INSUFFICIENT_HISTORY ) {
         throw insufficientHistory("ALMA open", inReal.length, startIdx, almaLookback(optInTimePeriod, optInSigma, optInOffset));
      }
      throw streamFailure("ALMA open", retCode);
   }
   /**
    * Open a live ALMA stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#alma} at that bar.
    * <p>The history must hold at least {@code almaLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@link Integer#MIN_VALUE} and {@link Core#REAL_DEFAULT} select a
    * parameter's documented default, as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public AlmaStream almaOpen( double inReal[], int optInTimePeriod, double optInSigma, double optInOffset )
   {
      requireArgument("ALMA open", "inReal", inReal);
      requireHistory("ALMA open", inReal.length);
      return almaOpenInternal(inReal, 0, optInTimePeriod, optInSigma, optInOffset);
   }
   /**
    * {@link Core#almaOpen} that also fills the output array(s) bit-identically
    * to {@link Core#alma} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link AlmaStream#outRange()}.
    */
   public AlmaStream almaOpenAndFill( double inReal[], int optInTimePeriod, double optInSigma, double optInOffset, double outReal[] )
   {
      requireArgument("ALMA openAndFill", "inReal", inReal);
      requireHistory("ALMA openAndFill", inReal.length);
      int guardOutLen = openFillCount("ALMA openAndFill", inReal.length, almaLookback(optInTimePeriod, optInSigma, optInOffset));
      requireLength("ALMA openAndFill", "outReal", outReal, guardOutLen);
      if( (Object)outReal == (Object)inReal ) {
         throw streamFailure("ALMA openAndFill", RetCode.BAD_PARAM);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return almaOpenAndFillInternal(inReal, 0, optInTimePeriod, optInSigma, optInOffset, outBegIdx, outNBElement, outReal);
   }
   private double almaStepTape( AlmaStream sp, double[] tape, int tapeBase, int tapeMask, double inReal )
   {
      double sum = 0.0;
      int j = 0;
      int w = 0;
      sum = 0.0;
      w = 0;
      for( j = sp.lookbackTotal; j >= 0; j -= 1 ) {
         sum += sp.cb_weights[w] * tape[(tapeBase - j) & tapeMask];
         w += 1;
      }
      sp.cur_outReal = sum;
      sp.outRangeCount++;
      return sp.cur_outReal;
   }
   private double almaPeekTape( AlmaStream sp, double[] tape, int tapeBase, int tapeMask, double inReal )
   {
      double sum = 0.0;
      int j = 0;
      int w = 0;
      double cur_outReal = 0.0;
      int pkSlot0 = -1;
      double pkVal0 = 0.0;
      pkSlot0 = tapeBase & tapeMask;
      pkVal0 = inReal;
      sum = 0.0;
      w = 0;
      for( j = sp.lookbackTotal; j >= 0; j -= 1 ) {
         sum += sp.cb_weights[w] * ((((tapeBase - j) & tapeMask) != pkSlot0) ? tape[(tapeBase - j) & tapeMask] : pkVal0);
         w += 1;
      }
      cur_outReal = sum;
      return cur_outReal;
   }
   private int almaTapeDetach( AlmaStream sp )
   {
      int reach = 0;
      sp.win_j_inReal = new double[0];
      if( sp.winCap_j - 1 > reach ) {
         reach = sp.winCap_j - 1;
      }
      return reach;
   }
