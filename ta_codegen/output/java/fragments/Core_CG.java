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
 *  092526 MF,CC  Initial version (#450).
 */

   /**
    * Number of leading input bars {@link Core#cg} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInTimePeriod Number of bars in the window (default 10; range
    *        2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int cgLookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 10;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return -1;
      }
      return optInTimePeriod - 1 ;

   }
   RetCode cgImpl( int startIdx,
                   int endIdx,
                   double inReal[],
                   int optInTimePeriod,
                   MInteger outBegIdx,
                   MInteger outNBElement,
                   double outReal[] )
   {
      int today = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      int i = 0;
      double num = 0;
      double den = 0;
      if( (startIdx < 0) || (startIdx > INDEX_MAX) ) {
         return RetCode.OUT_OF_RANGE_START_INDEX ;
      }
      if( (endIdx < 0) || (endIdx > INDEX_MAX) || (endIdx < startIdx)) {
         return RetCode.OUT_OF_RANGE_END_INDEX ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 10;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      outBegIdx.value = 0;
      outNBElement.value = 0;
      lookbackTotal = cgLookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         return RetCode.SUCCESS ;
      }
      outIdx = 0;
      today = startIdx;
      while( today <= endIdx ) {
         /* Oldest first, the value i bars ago is in den for the last i+1
          * additions to num, which is its weight in the listing. Walking the
          * window newest first would reverse every weight.
          */
         num = 0.0;
         den = 0.0;
         for( i = optInTimePeriod - 1; i >= 0; i -= 1 ) {
            den += inReal[today - i];
            num += den;
         }
         /* The denominator is a signed sum, so only an exact zero is degenerate.
          * It is answered with the flat-window value, which keeps every output a
          * function of its own window; an epsilon band would carry the quote unit
          * (#253).
          */
         if( den != 0.0 ) {
            outReal[outIdx] = (0 - num) / den;
         } else {
            outReal[outIdx] = (0 - ((double)optInTimePeriod + 1.0)) * 0.5;
         }
         outIdx += 1;
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.SUCCESS ;
   }
   RetCode cgImpl( int startIdx,
                   int endIdx,
                   float inReal[],
                   int optInTimePeriod,
                   MInteger outBegIdx,
                   MInteger outNBElement,
                   double outReal[] )
   {
      int today = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      int i = 0;
      double num = 0;
      double den = 0;
      if( (startIdx < 0) || (startIdx > INDEX_MAX) ) {
         return RetCode.OUT_OF_RANGE_START_INDEX ;
      }
      if( (endIdx < 0) || (endIdx > INDEX_MAX) || (endIdx < startIdx)) {
         return RetCode.OUT_OF_RANGE_END_INDEX ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 10;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      outBegIdx.value = 0;
      outNBElement.value = 0;
      lookbackTotal = cgLookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         return RetCode.SUCCESS ;
      }
      outIdx = 0;
      today = startIdx;
      while( today <= endIdx ) {
         num = 0.0;
         den = 0.0;
         for( i = optInTimePeriod - 1; i >= 0; i -= 1 ) {
            den += (double)inReal[today - i];
            num += den;
         }
         if( den != 0.0 ) {
            outReal[outIdx] = (0 - num) / den;
         } else {
            outReal[outIdx] = (0 - ((double)optInTimePeriod + 1.0)) * 0.5;
         }
         outIdx += 1;
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      return RetCode.SUCCESS ;
   }
   /**
    * John Ehlers' Center of Gravity oscillator: the balance point of the last
    * {@code optInTimePeriod} values, each weighted by its value and placed at
    * its position counting back from the current bar, which is position 1,
    * negated so that it rises with price. It is smooth and has essentially no
    * lag. Ehlers reads turning points from it and trades its crossings with a
    * copy of itself delayed by one bar. For a positive series it stays between
    * -optInTimePeriod and -1, and a flat window sits at the midpoint,
    * -(optInTimePeriod+1)/2.
    * <p>Formula and more info at <a
    * href="https://ta-lib.org/functions/cg">ta-lib.org/functions/cg</a>.
    * <p><b>Notes</b>
    * <ul>
    * <li>Ehlers' default input is the median price (H+L)/2: pass the output of MEDPRICE to reproduce it.</li>
    * <li>His 2004 book presents the same oscillator shifted up by (optInTimePeriod+1)/2, so that a flat window reads 0: add (optInTimePeriod+1)/2 to the output to obtain that form.</li>
    * <li>Where the window sums to exactly zero the author's listing keeps its previous value; TA-Lib returns -(optInTimePeriod+1)/2, so every value depends on its own window alone.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range that ends before {@link Core#cgLookback} is a <b>success with
    * no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal The series to measure.
    * @param optInTimePeriod Number of bars in the window (default 10; range
    *        2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outReal Negated center of gravity of the window. Must hold at least
    *        {@code endIdx - max(startIdx, cgLookback(...)) + 1} values, the count the
    *        call produces (none when that is not positive).
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
    * @see Core#medprice
    * @see Core#cti
    */
   public OutRange cg( int startIdx,
                       int endIdx,
                       double inReal[],
                       int optInTimePeriod,
                       double outReal[] )
   {
      requireIndexRange("CG", startIdx, endIdx);
      int guardStart = clampedStart("CG", startIdx, cgLookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("CG", "inReal", inReal, guardInLen);
      requireLength("CG", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = cgImpl(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.SUCCESS ) {
         throw failure("CG", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * John Ehlers' Center of Gravity oscillator: the balance point of the last
    * {@code optInTimePeriod} values, each weighted by its value and placed at
    * its position counting back from the current bar, which is position 1,
    * negated so that it rises with price. It is smooth and has essentially no
    * lag. Ehlers reads turning points from it and trades its crossings with a
    * copy of itself delayed by one bar. For a positive series it stays between
    * -optInTimePeriod and -1, and a flat window sits at the midpoint,
    * -(optInTimePeriod+1)/2.
    * <p>Formula and more info at <a
    * href="https://ta-lib.org/functions/cg">ta-lib.org/functions/cg</a>.
    * <p><b>Notes</b>
    * <ul>
    * <li>Ehlers' default input is the median price (H+L)/2: pass the output of MEDPRICE to reproduce it.</li>
    * <li>His 2004 book presents the same oscillator shifted up by (optInTimePeriod+1)/2, so that a flat window reads 0: add (optInTimePeriod+1)/2 to the output to obtain that form.</li>
    * <li>Where the window sums to exactly zero the author's listing keeps its previous value; TA-Lib returns -(optInTimePeriod+1)/2, so every value depends on its own window alone.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range that ends before {@link Core#cgLookback} is a <b>success with
    * no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal The series to measure.
    * @param optInTimePeriod Number of bars in the window (default 10; range
    *        2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outReal Negated center of gravity of the window. Must hold at least
    *        {@code endIdx - max(startIdx, cgLookback(...)) + 1} values, the count the
    *        call produces (none when that is not positive).
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
    * @see Core#medprice
    * @see Core#cti
    */
   public OutRange cg( int startIdx,
                       int endIdx,
                       float inReal[],
                       int optInTimePeriod,
                       double outReal[] )
   {
      requireIndexRange("CG", startIdx, endIdx);
      int guardStart = clampedStart("CG", startIdx, cgLookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("CG", "inReal", inReal, guardInLen);
      requireLength("CG", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = cgImpl(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.SUCCESS ) {
         throw failure("CG", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live CG stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#cg} over the same series.
    * Open with {@link Core#cgOpen}; there is no close — the handle is
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
   public static final class CgStream {
      private Core core;
      private int optInTimePeriod;
      private int winPos_i;
      private int winCap_i;
      private double[] win_i_inReal;
      private double cur_outReal;
      private int outRangeBegIdx;
      private int outRangeCount;

      private CgStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#cg} reports over the same bars: the
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
            throw failure("CG advance", RetCode.OUT_OF_RANGE_END_INDEX);
         this.outRangeCount++;
      }

      private CgStream( CgStream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.winPos_i = other.winPos_i;
         this.winCap_i = other.winCap_i;
         this.win_i_inReal = other.win_i_inReal.clone();
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
            throw failure("CG update", RetCode.OUT_OF_RANGE_END_INDEX);
         if( !Double.isFinite(inReal) )
            throw nonFiniteBar("CG update", "inReal");
         core.cgStepImpl(this, inReal);
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
            throw nonFiniteBar("CG peek", "inReal");
         CgStream sp = this;
         int i = 0;
         double num = 0.0;
         double den = 0.0;
         double cur_outReal = 0.0;
         int pkSlot0 = -1;
         double pkVal0 = 0.0;
         pkSlot0 = sp.winPos_i;
         pkVal0 = inReal;
         /* Oldest first, the value i bars ago is in den for the last i+1
          * additions to num, which is its weight in the listing. Walking the
          * window newest first would reverse every weight.
          */
         num = 0.0;
         den = 0.0;
         for( i = sp.optInTimePeriod - 1; i >= 0; i -= 1 ) {
            den += (((sp.winPos_i + sp.winCap_i - i >= sp.winCap_i) ? sp.winPos_i + sp.winCap_i - i - sp.winCap_i : sp.winPos_i + sp.winCap_i - i) != pkSlot0) ? sp.win_i_inReal[(sp.winPos_i + sp.winCap_i - i >= sp.winCap_i) ? sp.winPos_i + sp.winCap_i - i - sp.winCap_i : sp.winPos_i + sp.winCap_i - i] : pkVal0;
            num += den;
         }
         /* The denominator is a signed sum, so only an exact zero is degenerate.
          * It is answered with the flat-window value, which keeps every output a
          * function of its own window; an epsilon band would carry the quote unit
          * (#253).
          */
         if( den != 0.0 ) {
            cur_outReal = (0 - num) / den;
         } else {
            cur_outReal = (0 - ((double)sp.optInTimePeriod + 1.0)) * 0.5;
         }
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
      public CgStream clone() {
         return new CgStream(this);
      }
   }
   private void cgStepImpl( CgStream sp, double inReal )
   {
      int i = 0;
      double num = 0.0;
      double den = 0.0;
      sp.win_i_inReal[sp.winPos_i] = inReal;
      /* Oldest first, the value i bars ago is in den for the last i+1
       * additions to num, which is its weight in the listing. Walking the
       * window newest first would reverse every weight.
       */
      num = 0.0;
      den = 0.0;
      for( i = sp.optInTimePeriod - 1; i >= 0; i -= 1 ) {
         den += sp.win_i_inReal[(sp.winPos_i + sp.winCap_i - i >= sp.winCap_i) ? sp.winPos_i + sp.winCap_i - i - sp.winCap_i : sp.winPos_i + sp.winCap_i - i];
         num += den;
      }
      /* The denominator is a signed sum, so only an exact zero is degenerate.
       * It is answered with the flat-window value, which keeps every output a
       * function of its own window; an epsilon band would carry the quote unit
       * (#253).
       */
      if( den != 0.0 ) {
         sp.cur_outReal = (0 - num) / den;
      } else {
         sp.cur_outReal = (0 - ((double)sp.optInTimePeriod + 1.0)) * 0.5;
      }
      sp.winPos_i = sp.winPos_i + 1;
      if( sp.winPos_i >= sp.winCap_i ) {
         sp.winPos_i = 0;
      }
   }
   private RetCode cgOpenImpl( CgStream sp, double inReal[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
   {
      int today = 0;
      int outIdx = 0;
      int lookbackTotal = 0;
      int i = 0;
      double num = 0;
      double den = 0;
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.OUT_OF_RANGE_START_INDEX;
      }
      if( historyLen > INDEX_MAX + 1 ) {
         return RetCode.OUT_OF_RANGE_END_INDEX;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 10;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.INSUFFICIENT_HISTORY;
      }
      outBegIdx.value = 0;
      outNBElement.value = 0;
      lookbackTotal = cgLookback(optInTimePeriod);
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         return RetCode.INSUFFICIENT_HISTORY ;
      }
      outIdx = 0;
      today = startIdx;
      while( today <= endIdx ) {
         /* Oldest first, the value i bars ago is in den for the last i+1
          * additions to num, which is its weight in the listing. Walking the
          * window newest first would reverse every weight.
          */
         num = 0.0;
         den = 0.0;
         for( i = optInTimePeriod - 1; i >= 0; i -= 1 ) {
            den += inReal[today - i];
            num += den;
         }
         /* The denominator is a signed sum, so only an exact zero is degenerate.
          * It is answered with the flat-window value, which keeps every output a
          * function of its own window; an epsilon band would carry the quote unit
          * (#253).
          */
         if( den != 0.0 ) {
            outReal[outIdx * outStride] = (0 - num) / den;
         } else {
            outReal[outIdx * outStride] = (0 - ((double)optInTimePeriod + 1.0)) * 0.5;
         }
         outIdx += 1;
         today += 1;
      }
      outBegIdx.value = startIdx;
      outNBElement.value = outIdx;
      /* Capture the live batch state into the handle. */
      int cap_i = (int)(optInTimePeriod - 1 + 1);
      if( cap_i < 1 || cap_i > historyLen ) {
         return RetCode.INTERNAL_ERROR;
      }
      double[] capWin_i_inReal = new double[cap_i];
      System.arraycopy(inReal, historyLen - cap_i, capWin_i_inReal, 0, cap_i);
      sp.optInTimePeriod = optInTimePeriod;
      sp.winPos_i = 0;
      sp.winCap_i = cap_i;
      sp.win_i_inReal = capWin_i_inReal;
      sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
      return RetCode.SUCCESS;
   }
   /* cgOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   CgStream cgOpenAndFillInternal( double inReal[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      CgStream sp = new CgStream(this);
      RetCode retCode = cgOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.SUCCESS ) {
         return sp;
      }
      if( retCode == RetCode.INSUFFICIENT_HISTORY ) {
         throw insufficientHistory("CG openAndFill", inReal.length, startIdx, cgLookback(optInTimePeriod));
      }
      throw streamFailure("CG openAndFill", retCode);
   }
   /* Internal startIdx-anchored open behind cgOpen (composition seam). */
   CgStream cgOpenInternal( double inReal[], int startIdx, int optInTimePeriod )
   {
      CgStream sp = new CgStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      RetCode retCode = cgOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.SUCCESS ) {
         return sp;
      }
      if( retCode == RetCode.INSUFFICIENT_HISTORY ) {
         throw insufficientHistory("CG open", inReal.length, startIdx, cgLookback(optInTimePeriod));
      }
      throw streamFailure("CG open", retCode);
   }
   /**
    * Open a live CG stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#cg} at that bar.
    * <p>The history must hold at least {@code cgLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@link Integer#MIN_VALUE} selects a parameter's documented default,
    * as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public CgStream cgOpen( double inReal[], int optInTimePeriod )
   {
      requireArgument("CG open", "inReal", inReal);
      requireHistory("CG open", inReal.length);
      return cgOpenInternal(inReal, 0, optInTimePeriod);
   }
   /**
    * {@link Core#cgOpen} that also fills the output array(s) bit-identically
    * to {@link Core#cg} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link CgStream#outRange()}.
    */
   public CgStream cgOpenAndFill( double inReal[], int optInTimePeriod, double outReal[] )
   {
      requireArgument("CG openAndFill", "inReal", inReal);
      requireHistory("CG openAndFill", inReal.length);
      int guardOutLen = openFillCount("CG openAndFill", inReal.length, cgLookback(optInTimePeriod));
      requireLength("CG openAndFill", "outReal", outReal, guardOutLen);
      if( (Object)outReal == (Object)inReal ) {
         throw streamFailure("CG openAndFill", RetCode.BAD_PARAM);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return cgOpenAndFillInternal(inReal, 0, optInTimePeriod, outBegIdx, outNBElement, outReal);
   }
