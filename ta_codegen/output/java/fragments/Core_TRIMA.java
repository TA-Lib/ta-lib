/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  CR       Chris (crokusek@hotmail.com)
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  010503 MF     Initial Coding
 *  031703 MF     Fix #701060. Correct logic when using a range with
 *                startIdx/endIdx. Thanks to Chris for reporting this.
 *  052603 MF     Adapt code to compile with .NET Managed C++
 *  071226 MF,CC  Widen the triangular-weight factor to double: (i+1)*(i+1)
 *                and i*(i+1) overflowed a 32-bit int at extreme periods
 *                (past ~92682), silently returning garbage. Bit-identical
 *                for every period where the int product fits.
 */

   /**
    * Number of leading input bars {@link Core#TRIMA} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInTimePeriod Number of bars in the averaging window (default 30;
    *        range 1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int TRIMA_Lookback( int optInTimePeriod )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return -1;
      }
      return optInTimePeriod - 1 ;

   }
   RetCode TRIMA_Impl( int startIdx,
                       int endIdx,
                       double inReal[],
                       int optInTimePeriod,
                       MInteger outBegIdx,
                       MInteger outNBElement,
                       double outReal[] )
   {
      int lookbackTotal = 0;
      double numerator = 0;
      double numeratorSub = 0;
      double numeratorAdd = 0;
      int i = 0;
      int outIdx = 0;
      int todayIdx = 0;
      int trailingIdx = 0;
      int middleIdx = 0;
      double factor = 0;
      double tempReal = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      /* Identify the minimum number of price bar needed
       * to calculate at least one output.
       */
      lookbackTotal = optInTimePeriod - 1;
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
      /* TRIMA Description
       * =================
       * The triangular MA is a weighted moving average. Instead of the
       * TA_WMA who put more weigth on the latest price bar, the triangular
       * put more weigth on the data in the middle of the specified period.
       *
       * Examples:
       *   For TimeSerie={a,b,c,d,e,f...} ('a' is the older price)
       *
       *   1st value for TRIMA 4-Period is:  ((1*a)+(2*b)+(2*c)+(1*d)) / 6
       *   2nd value for TRIMA 4-Period is:  ((1*b)+(2*c)+(2*d)+(1*e)) / 6
       *
       *   1st value for TRIMA 5-Period is:  ((1*a)+(2*b)+(3*c)+(2*d)+(1*e)) / 9
       *   2nd value for TRIMA 5-Period is:  ((1*b)+(2*c)+(3*d)+(2*e)+(1*f)) / 9
       *
       * Generally Accepted Implementation
       * ==================================
       * Using algebra, it can be demonstrated that the TRIMA is equivalent to
       * doing a SMA of a SMA. The following explain the rules:
       *
       *  (1) When the period is even, TRIMA(x,period)=SMA(SMA(x,period/2),(period/2)+1)
       *  (2) When the period is odd,  TRIMA(x,period)=SMA(SMA(x,(period+1)/2),(period+1)/2)
       *
       * In other word:
       *  (1) A period of 4 becomes TRIMA(x,4) = SMA( SMA( x, 2), 3 )
       *  (2) A period of 5 becomes TRIMA(x,5) = SMA( SMA( x, 3), 3 )
       *
       * The SMA of a SMA is the algorithm generaly found in books.
       *
       * Tradestation Implementation
       * ===========================
       * Tradestation deviate from the generally accepted implementation by
       * making the TRIMA to be as follow:
       *    TRIMA(x,period) = SMA( SMA( x, (int)(period/2)+1), (int)(period/2)+1 );
       * This formula is done regardless if the period is even or odd.
       *
       * In other word:
       *  (1) A period of 4 becomes TRIMA(x,4) = SMA( SMA( x, 3), 3 )
       *  (2) A period of 5 becomes TRIMA(x,5) = SMA( SMA( x, 3), 3 )
       *  (3) A period of 6 becomes TRIMA(x,5) = SMA( SMA( x, 4), 4 )
       *  (4) A period of 7 becomes TRIMA(x,5) = SMA( SMA( x, 4), 4 )
       *
       * It is not clear to me if the Tradestation approach is a bug or a deliberate
       * decision to do things differently.
       *
       * Metastock Implementation
       * ========================
       * Output is the same as the generally accepted implementation.
       *
       * TA-Lib Implementation
       * =====================
       * Output is also the same as the generally accepted implementation.
       *
       * For speed optimization and avoid memory allocation, TA-Lib use
       * a better algorithm than the usual SMA of a SMA.
       *
       * The calculation from one TRIMA value to the next is done by doing 4
       * little adjustment (the following show a TRIMA 4-period):
       *
       * TRIMA at time 'd': ((1*a)+(2*b)+(2*c)+(1*d)) / 6
       * TRIMA at time 'e': ((1*b)+(2*c)+(2*d)+(1*e)) / 6
       *
       * To go from TRIMA 'd' to 'e', the following is done:
       *       1) 'a' and 'b' are substract from the numerator.
       *       2) 'd' is added to the numerator.
       *       3) 'e' is added to the numerator.
       *       4) Calculate TRIMA by doing numerator / 6
       *       5) Repeat sequence for next output
       *
       * These operations are the same steps done by TA-LIB:
       *       1) is done by numeratorSub
       *       2) is done by numeratorAdd.
       *       3) is obtain from the latest input
       *       4) Calculate and write TRIMA in the output
       *       5) Repeat for next output.
       *
       * Of course, numerotrAdd and numeratorSub needs to be
       * adjusted for each iteration.
       *
       * The update of numeratorSub needs values from the input at
       * the trailingIdx and middleIdx position.
       *
       * The update of numeratorAdd needs values from the input at
       * the middleIdx and todayIdx.
       */
      outIdx = 0;
      if( optInTimePeriod % 2 == 1 ) {
         /* Logic for Odd period */
         /* Calculate the factor which is 1 divided by the
          * sumation of the weight.
          *
          * The sum of the weight is calculated as follow:
          *
          * The simple sumation serie 1+2+3... n can be
          * express as n(n+1)/2
          *
          * From this logic, a "triangular" sumation formula
          * can be found depending if the period is odd or even.
          *
          * Odd Period Formula:
          *  period = 5 and with n=(int)(period/2)
          *  the formula for a "triangular" serie is:
          *    1+2+3+2+1 = (n*(n+1))+n+1
          *              = (n+1)*(n+1)
          *              = 3 * 3 = 9
          *
          * Even period Formula:
          *   period = 6 and with n=(int)(period/2)
          *   the formula for a "triangular" serie is:
          *    1+2+3+3+2+1 = n*(n+1)
          *                = 3 * 4 = 12
          */
         /* Note: the (i+1) factors are widened to double so the product
          *       cannot overflow a 32-bit int at extreme periods (i+1 reaches
          *       ~50000 near the API maximum, and (i+1)*(i+1) exceeds INT_MAX
          *       past period ~92682). For every period where the int product
          *       fits, the widened value is identical.
          */
         i = optInTimePeriod >> 1;
         factor = (double)(i + 1) * (i + 1);
         factor = 1.0 / factor;
         /* Initialize all the variable before
          * starting to iterate for each output.
          */
         trailingIdx = startIdx - lookbackTotal;
         middleIdx = trailingIdx + i;
         todayIdx = middleIdx + i;
         numerator = 0.0;
         numeratorSub = 0.0;
         for( i = middleIdx; i >= trailingIdx; i -= 1 ) {
            tempReal = inReal[i];
            numeratorSub += tempReal;
            numerator += numeratorSub;
         }
         numeratorAdd = 0.0;
         middleIdx += 1;
         for( i = middleIdx; i <= todayIdx; i += 1 ) {
            tempReal = inReal[i];
            numeratorAdd += tempReal;
            numerator += numeratorAdd;
         }
         /* Write the first output */
         outIdx = 0;
         tempReal = inReal[trailingIdx++];
         outReal[outIdx++] = numerator * factor;
         todayIdx += 1;
         /* Note: The value at the trailingIdx was saved
          *       in tempReal to account for the case where
          *       outReal and inReal are ptr on the same
          *       buffer.
          */
         /* Iterate for remaining output */
         while( todayIdx <= endIdx ) {
            /* Step (1) */
            numerator -= numeratorSub;
            numeratorSub -= tempReal;
            tempReal = inReal[middleIdx++];
            numeratorSub += tempReal;
            /* Step (2) */
            numerator += numeratorAdd;
            numeratorAdd -= tempReal;
            tempReal = inReal[todayIdx++];
            numeratorAdd += tempReal;
            /* Step (3) */
            numerator += tempReal;
            /* Step (4) */
            tempReal = inReal[trailingIdx++];
            outReal[outIdx++] = numerator * factor;
         }
      } else {
         /* Even logic.
          *
          * Very similar to the odd logic, except:
          *  - calculation of the factor is different.
          *  - the coverage of the numeratorSub and numeratorAdd is
          *    slightly different.
          *  - Adjustment of numeratorAdd is different. See Step (2).
          */
         i = optInTimePeriod >> 1;
         factor = (double)i * (i + 1);
         /* widen: i*(i+1) overflows int past period ~92682 */
         factor = 1.0 / factor;
         /* Initialize all the variable before
          * starting to iterate for each output.
          */
         trailingIdx = startIdx - lookbackTotal;
         middleIdx = trailingIdx + i - 1;
         todayIdx = middleIdx + i;
         numerator = 0.0;
         numeratorSub = 0.0;
         for( i = middleIdx; i >= trailingIdx; i -= 1 ) {
            tempReal = inReal[i];
            numeratorSub += tempReal;
            numerator += numeratorSub;
         }
         numeratorAdd = 0.0;
         middleIdx += 1;
         for( i = middleIdx; i <= todayIdx; i += 1 ) {
            tempReal = inReal[i];
            numeratorAdd += tempReal;
            numerator += numeratorAdd;
         }
         /* Write the first output */
         outIdx = 0;
         tempReal = inReal[trailingIdx++];
         outReal[outIdx++] = numerator * factor;
         todayIdx += 1;
         /* Note: The value at the trailingIdx was saved
          *       in tempReal to account for the case where
          *       outReal and inReal are ptr on the same
          *       buffer.
          */
         /* Iterate for remaining output */
         while( todayIdx <= endIdx ) {
            /* Step (1) */
            numerator -= numeratorSub;
            numeratorSub -= tempReal;
            tempReal = inReal[middleIdx++];
            numeratorSub += tempReal;
            /* Step (2) */
            numeratorAdd -= tempReal;
            numerator += numeratorAdd;
            tempReal = inReal[todayIdx++];
            numeratorAdd += tempReal;
            /* Step (3) */
            numerator += tempReal;
            /* Step (4) */
            tempReal = inReal[trailingIdx++];
            outReal[outIdx++] = numerator * factor;
         }
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   RetCode TRIMA_Impl( int startIdx,
                       int endIdx,
                       float inReal[],
                       int optInTimePeriod,
                       MInteger outBegIdx,
                       MInteger outNBElement,
                       double outReal[] )
   {
      int lookbackTotal = 0;
      double numerator = 0;
      double numeratorSub = 0;
      double numeratorAdd = 0;
      int i = 0;
      int outIdx = 0;
      int todayIdx = 0;
      int trailingIdx = 0;
      int middleIdx = 0;
      double factor = 0;
      double tempReal = 0;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      lookbackTotal = optInTimePeriod - 1;
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.Success ;
      }
      outIdx = 0;
      if( optInTimePeriod % 2 == 1 ) {
         i = optInTimePeriod >> 1;
         factor = (double)(i + 1) * (i + 1);
         factor = 1.0 / factor;
         trailingIdx = startIdx - lookbackTotal;
         middleIdx = trailingIdx + i;
         todayIdx = middleIdx + i;
         numerator = 0.0;
         numeratorSub = 0.0;
         for( i = middleIdx; i >= trailingIdx; i -= 1 ) {
            tempReal = (double)inReal[i];
            numeratorSub += tempReal;
            numerator += numeratorSub;
         }
         numeratorAdd = 0.0;
         middleIdx += 1;
         for( i = middleIdx; i <= todayIdx; i += 1 ) {
            tempReal = (double)inReal[i];
            numeratorAdd += tempReal;
            numerator += numeratorAdd;
         }
         outIdx = 0;
         tempReal = (double)inReal[trailingIdx++];
         outReal[outIdx++] = numerator * factor;
         todayIdx += 1;
         while( todayIdx <= endIdx ) {
            numerator -= numeratorSub;
            numeratorSub -= tempReal;
            tempReal = (double)inReal[middleIdx++];
            numeratorSub += tempReal;
            numerator += numeratorAdd;
            numeratorAdd -= tempReal;
            tempReal = (double)inReal[todayIdx++];
            numeratorAdd += tempReal;
            numerator += tempReal;
            tempReal = (double)inReal[trailingIdx++];
            outReal[outIdx++] = numerator * factor;
         }
      } else {
         i = optInTimePeriod >> 1;
         factor = (double)i * (i + 1);
         factor = 1.0 / factor;
         trailingIdx = startIdx - lookbackTotal;
         middleIdx = trailingIdx + i - 1;
         todayIdx = middleIdx + i;
         numerator = 0.0;
         numeratorSub = 0.0;
         for( i = middleIdx; i >= trailingIdx; i -= 1 ) {
            tempReal = (double)inReal[i];
            numeratorSub += tempReal;
            numerator += numeratorSub;
         }
         numeratorAdd = 0.0;
         middleIdx += 1;
         for( i = middleIdx; i <= todayIdx; i += 1 ) {
            tempReal = (double)inReal[i];
            numeratorAdd += tempReal;
            numerator += numeratorAdd;
         }
         outIdx = 0;
         tempReal = (double)inReal[trailingIdx++];
         outReal[outIdx++] = numerator * factor;
         todayIdx += 1;
         while( todayIdx <= endIdx ) {
            numerator -= numeratorSub;
            numeratorSub -= tempReal;
            tempReal = (double)inReal[middleIdx++];
            numeratorSub += tempReal;
            numeratorAdd -= tempReal;
            numerator += numeratorAdd;
            tempReal = (double)inReal[todayIdx++];
            numeratorAdd += tempReal;
            numerator += tempReal;
            tempReal = (double)inReal[trailingIdx++];
            outReal[outIdx++] = numerator * factor;
         }
      }
      outNBElement.value = outIdx;
      outBegIdx.value = startIdx;
      return RetCode.Success ;
   }
   /**
    * Triangular Moving Average: a double-smoothed moving average that weights
    * prices toward the middle of the window most heavily. Equivalent to an SMA
    * of an SMA, computed here via an incremental triangular-weighted running
    * numerator.
    * <p><b>Formula</b>
    * <pre>{@code
    * Weights rise then fall (4-period: (1a+2b+2c+1d)/6; 5-period: (1a+2b+3c+2d+1e)/9). With n = period>>1: odd divides by (n+1)^2, even by n(n+1). Equivalent to odd: SMA(SMA(x,(period+1)/2),(period+1)/2); even: SMA(SMA(x,period/2),period/2+1).
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>Follows the generally accepted (Metastock) definition rather than the TradeStation variant.</li>
    * <li>A period of 1 performs no smoothing: the output is a copy of the input. Allowed since 0.6.5 (issues #48/#59).</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#TRIMA_Lookback} is a <b>success with
    * no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Source price series.
    * @param optInTimePeriod Number of bars in the averaging window (default 30;
    *        range 1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outReal Triangular moving average. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#MAX_INDEX}, or {@code endIdx < startIdx}.
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
    * @see Core#SMA
    * @see Core#WMA
    * @see Core#MA
    */
   public OutRange TRIMA( int startIdx,
                          int endIdx,
                          double inReal[],
                          int optInTimePeriod,
                          double outReal[] )
   {
      requireIndexRange("TRIMA", startIdx, endIdx);
      int guardStart = clampedStart("TRIMA", startIdx, TRIMA_Lookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("TRIMA", "inReal", inReal, guardInLen);
      requireLength("TRIMA", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = TRIMA_Impl(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("TRIMA", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Triangular Moving Average: a double-smoothed moving average that weights
    * prices toward the middle of the window most heavily. Equivalent to an SMA
    * of an SMA, computed here via an incremental triangular-weighted running
    * numerator.
    * <p><b>Formula</b>
    * <pre>{@code
    * Weights rise then fall (4-period: (1a+2b+2c+1d)/6; 5-period: (1a+2b+3c+2d+1e)/9). With n = period>>1: odd divides by (n+1)^2, even by n(n+1). Equivalent to odd: SMA(SMA(x,(period+1)/2),(period+1)/2); even: SMA(SMA(x,period/2),period/2+1).
    * }</pre>
    * <p><b>Notes</b>
    * <ul>
    * <li>Follows the generally accepted (Metastock) definition rather than the TradeStation variant.</li>
    * <li>A period of 1 performs no smoothing: the output is a copy of the input. Allowed since 0.6.5 (issues #48/#59).</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#TRIMA_Lookback} is a <b>success with
    * no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Source price series.
    * @param optInTimePeriod Number of bars in the averaging window (default 30;
    *        range 1..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param outReal Triangular moving average. Must hold at least
    *        {@code endIdx - startIdx + 1} values.
    * @return The range written: {@code begIdx} is the first bar with a value,
    *        {@code count} how many were written.
    * @throws IndexOutOfBoundsException if {@code startIdx} or {@code endIdx} is
    *        negative or above {@link Core#MAX_INDEX}, or {@code endIdx < startIdx}.
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
    * @see Core#SMA
    * @see Core#WMA
    * @see Core#MA
    */
   public OutRange TRIMA( int startIdx,
                          int endIdx,
                          float inReal[],
                          int optInTimePeriod,
                          double outReal[] )
   {
      requireIndexRange("TRIMA", startIdx, endIdx);
      int guardStart = clampedStart("TRIMA", startIdx, TRIMA_Lookback(optInTimePeriod));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("TRIMA", "inReal", inReal, guardInLen);
      requireLength("TRIMA", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = TRIMA_Impl(startIdx, endIdx, inReal, optInTimePeriod, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.Success ) {
         throw failure("TRIMA", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live TRIMA stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#TRIMA} over the same series.
    * Open with {@link Core#trimaOpen}; there is no close — the handle is
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
   public static final class TrimaStream {
      Core core;
      int optInTimePeriod;
      double numerator;
      double numeratorSub;
      double numeratorAdd;
      double factor;
      double tempReal;
      int ringPos_middleIdx;
      int ringCap_middleIdx;
      double[] ring_middleIdx_inReal;
      int ringPos_trailingIdx;
      int ringCap_trailingIdx;
      double[] ring_trailingIdx_inReal;
      double cur_outReal;
      int outRangeBegIdx;
      int outRangeCount;

      TrimaStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#TRIMA} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * {@code update} adds one to the count — a bar rejected for being
       * non-finite included, because it still happened — {@code peek} leaves
       * it alone, and {@code clone()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       */
      public OutRange outRange() { return new OutRange(outRangeBegIdx, outRangeCount); }

      TrimaStream( TrimaStream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.numerator = other.numerator;
         this.numeratorSub = other.numeratorSub;
         this.numeratorAdd = other.numeratorAdd;
         this.factor = other.factor;
         this.tempReal = other.tempReal;
         this.ringPos_middleIdx = other.ringPos_middleIdx;
         this.ringCap_middleIdx = other.ringCap_middleIdx;
         this.ring_middleIdx_inReal = other.ring_middleIdx_inReal.clone();
         this.ringPos_trailingIdx = other.ringPos_trailingIdx;
         this.ringCap_trailingIdx = other.ringCap_trailingIdx;
         this.ring_trailingIdx_inReal = other.ring_trailingIdx_inReal.clone();
         this.cur_outReal = other.cur_outReal;
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      /**
       * Commit one closed bar, returning the new current value.
       * Never allocates handle state.
       * <p>Throws {@link IllegalArgumentException} if any bar value is not
       * finite (NaN or an infinity). That check runs before anything is
       * written, so the state is left exactly as it was: the rejected bar's
       * output is the previous value, held, and {@link #value()} answers it.
       * The stream stays usable, so skip the bar or re-open on a clean
       * history. {@link #outRange()} does advance: the bar happened and
       * occupies a position in the series, so the handle counts it, which is
       * what keeps two handles on one feed aligned when only one rejects.
       * This is the one place the streaming tier is stricter than
       * the batch API, which computes on whatever it is given: a handle
       * retains its state, so a single non-finite bar would poison every
       * later value it produces.
       */
      public double update( double inReal ) {
         if( !Double.isFinite(inReal) ) {
            if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
            throw new TaLibArgumentException("TRIMA update: BadParam", RetCode.BadParam);
         }
         core.trimaStepImpl(this, inReal);
         if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         return this.cur_outReal;
      }

      /**
       * Commit {@code n} closed bars and write their {@code n} values, in one
       * call — exactly {@code n} back-to-back {@code update} calls, with one
       * set of argument checks instead of {@code n}. {@code n} is
       * {@code inReal.length}; the outputs must hold at least that many, and must
       * not be the same array as an input or as each other.
       * <p>{@link #outRange()} counts what this call took in, which is what makes a
       * rejection readable: a non-finite bar {@code k} throws
       * {@link IllegalArgumentException} exactly as {@code update} would, with
       * the bars before {@code k} committed and written, bar {@code k} and
       * everything after it not, and the count advanced by {@code k + 1} —
       * the committed bars plus the rejected one.
       */
      public void updateAndFill( double inReal[], double outReal[] ) {
         requireArgument("TRIMA updateAndFill", "inReal", inReal);
         requireArgument("TRIMA updateAndFill", "outReal", outReal);
         final int barCount = inReal.length;
         if( outReal.length < barCount || (Object)outReal == (Object)inReal )
            throw new TaLibArgumentException("TRIMA updateAndFill: BadParam", RetCode.BadParam);
         for( int i = 0; i < barCount; i++ ) {
            if( !Double.isFinite(inReal[i]) ) {
               if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
               throw new TaLibArgumentException("TRIMA updateAndFill: BadParam", RetCode.BadParam);
            }
            core.trimaStepImpl(this, inReal[i]);
            outReal[i] = this.cur_outReal;
            if( this.outRangeCount < MAX_INDEX ) this.outRangeCount++;
         }
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return — the same
       * transition, with every store it would make carried in a local instead.
       * Never writes this handle, so peeks may
       * run concurrently with each other. It copies nothing: the frame runs against this handle, reading its
       * buffers and storing what the step would commit into locals, so the cost
       * does not grow with the period and {@code peek} never allocates.
       */
      public double peek( double inReal ) {
         if( !Double.isFinite(inReal) )
            throw new TaLibArgumentException("TRIMA peek: BadParam", RetCode.BadParam);
         TrimaStream sp = this;
         double cur_outReal = 0.0;
         if( sp.optInTimePeriod % 2 == 1 ) {
            double numerator = sp.numerator;
            double numeratorAdd = sp.numeratorAdd;
            double numeratorSub = sp.numeratorSub;
            int ringPos_middleIdx = sp.ringPos_middleIdx;
            int ringPos_trailingIdx = sp.ringPos_trailingIdx;
            double tempReal = sp.tempReal;
            int pkSlot0 = -1;
            double pkVal0 = 0.0;
            int pkSlot1 = -1;
            double pkVal1 = 0.0;
            if( sp.ringCap_middleIdx == 0 ) {
               pkSlot0 = 0;
               pkVal0 = inReal;
            }
            if( sp.ringCap_trailingIdx == 0 ) {
               pkSlot1 = 0;
               pkVal1 = inReal;
            }
            /* Step (1) */
            numerator -= numeratorSub;
            numeratorSub -= tempReal;
            tempReal = (ringPos_middleIdx != pkSlot0) ? sp.ring_middleIdx_inReal[ringPos_middleIdx] : pkVal0;
            numeratorSub += tempReal;
            /* Step (2) */
            numerator += numeratorAdd;
            numeratorAdd -= tempReal;
            tempReal = inReal;
            numeratorAdd += tempReal;
            /* Step (3) */
            numerator += tempReal;
            /* Step (4) */
            tempReal = (ringPos_trailingIdx != pkSlot1) ? sp.ring_trailingIdx_inReal[ringPos_trailingIdx] : pkVal1;
            cur_outReal = numerator * sp.factor;
            ringPos_middleIdx = ringPos_middleIdx + 1;
            if( ringPos_middleIdx >= sp.ringCap_middleIdx ) {
               ringPos_middleIdx = 0;
            }
            ringPos_trailingIdx = ringPos_trailingIdx + 1;
            if( ringPos_trailingIdx >= sp.ringCap_trailingIdx ) {
               ringPos_trailingIdx = 0;
            }
         } else {
            double numerator = sp.numerator;
            double numeratorAdd = sp.numeratorAdd;
            double numeratorSub = sp.numeratorSub;
            int ringPos_middleIdx = sp.ringPos_middleIdx;
            int ringPos_trailingIdx = sp.ringPos_trailingIdx;
            double tempReal = sp.tempReal;
            int pkSlot0 = -1;
            double pkVal0 = 0.0;
            int pkSlot1 = -1;
            double pkVal1 = 0.0;
            if( sp.ringCap_middleIdx == 0 ) {
               pkSlot0 = 0;
               pkVal0 = inReal;
            }
            if( sp.ringCap_trailingIdx == 0 ) {
               pkSlot1 = 0;
               pkVal1 = inReal;
            }
            /* Step (1) */
            numerator -= numeratorSub;
            numeratorSub -= tempReal;
            tempReal = (ringPos_middleIdx != pkSlot0) ? sp.ring_middleIdx_inReal[ringPos_middleIdx] : pkVal0;
            numeratorSub += tempReal;
            /* Step (2) */
            numeratorAdd -= tempReal;
            numerator += numeratorAdd;
            tempReal = inReal;
            numeratorAdd += tempReal;
            /* Step (3) */
            numerator += tempReal;
            /* Step (4) */
            tempReal = (ringPos_trailingIdx != pkSlot1) ? sp.ring_trailingIdx_inReal[ringPos_trailingIdx] : pkVal1;
            cur_outReal = numerator * sp.factor;
            ringPos_middleIdx = ringPos_middleIdx + 1;
            if( ringPos_middleIdx >= sp.ringCap_middleIdx ) {
               ringPos_middleIdx = 0;
            }
            ringPos_trailingIdx = ringPos_trailingIdx + 1;
            if( ringPos_trailingIdx >= sp.ringCap_trailingIdx ) {
               ringPos_trailingIdx = 0;
            }
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
      public TrimaStream clone() {
         return new TrimaStream(this);
      }
   }
   void trimaStepImpl( TrimaStream sp, double inReal )
   {
      if( sp.optInTimePeriod % 2 == 1 ) {
         if( sp.ringCap_middleIdx == 0 ) {
            sp.ring_middleIdx_inReal[0] = inReal;
         }
         if( sp.ringCap_trailingIdx == 0 ) {
            sp.ring_trailingIdx_inReal[0] = inReal;
         }
         /* Step (1) */
         sp.numerator -= sp.numeratorSub;
         sp.numeratorSub -= sp.tempReal;
         sp.tempReal = sp.ring_middleIdx_inReal[sp.ringPos_middleIdx];
         sp.numeratorSub += sp.tempReal;
         /* Step (2) */
         sp.numerator += sp.numeratorAdd;
         sp.numeratorAdd -= sp.tempReal;
         sp.tempReal = inReal;
         sp.numeratorAdd += sp.tempReal;
         /* Step (3) */
         sp.numerator += sp.tempReal;
         /* Step (4) */
         sp.tempReal = sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx];
         sp.cur_outReal = sp.numerator * sp.factor;
         sp.ring_middleIdx_inReal[sp.ringPos_middleIdx] = inReal;
         sp.ringPos_middleIdx = sp.ringPos_middleIdx + 1;
         if( sp.ringPos_middleIdx >= sp.ringCap_middleIdx ) {
            sp.ringPos_middleIdx = 0;
         }
         sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx] = inReal;
         sp.ringPos_trailingIdx = sp.ringPos_trailingIdx + 1;
         if( sp.ringPos_trailingIdx >= sp.ringCap_trailingIdx ) {
            sp.ringPos_trailingIdx = 0;
         }
      } else {
         if( sp.ringCap_middleIdx == 0 ) {
            sp.ring_middleIdx_inReal[0] = inReal;
         }
         if( sp.ringCap_trailingIdx == 0 ) {
            sp.ring_trailingIdx_inReal[0] = inReal;
         }
         /* Step (1) */
         sp.numerator -= sp.numeratorSub;
         sp.numeratorSub -= sp.tempReal;
         sp.tempReal = sp.ring_middleIdx_inReal[sp.ringPos_middleIdx];
         sp.numeratorSub += sp.tempReal;
         /* Step (2) */
         sp.numeratorAdd -= sp.tempReal;
         sp.numerator += sp.numeratorAdd;
         sp.tempReal = inReal;
         sp.numeratorAdd += sp.tempReal;
         /* Step (3) */
         sp.numerator += sp.tempReal;
         /* Step (4) */
         sp.tempReal = sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx];
         sp.cur_outReal = sp.numerator * sp.factor;
         sp.ring_middleIdx_inReal[sp.ringPos_middleIdx] = inReal;
         sp.ringPos_middleIdx = sp.ringPos_middleIdx + 1;
         if( sp.ringPos_middleIdx >= sp.ringCap_middleIdx ) {
            sp.ringPos_middleIdx = 0;
         }
         sp.ring_trailingIdx_inReal[sp.ringPos_trailingIdx] = inReal;
         sp.ringPos_trailingIdx = sp.ringPos_trailingIdx + 1;
         if( sp.ringPos_trailingIdx >= sp.ringCap_trailingIdx ) {
            sp.ringPos_trailingIdx = 0;
         }
      }
   }
   private RetCode trimaOpenImpl( TrimaStream sp, double inReal[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
   {
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.OutOfRangeStartIndex;
      }
      if( historyLen > MAX_INDEX + 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 30;
      } else if( optInTimePeriod < 1 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInTimePeriod % 2 == 1 ) {
         int lookbackTotal = 0;
         double numerator = 0;
         double numeratorSub = 0;
         double numeratorAdd = 0;
         int i = 0;
         int outIdx = 0;
         int todayIdx = 0;
         int trailingIdx = 0;
         int middleIdx = 0;
         double factor = 0;
         double tempReal = 0;
         /* Identify the minimum number of price bar needed
          * to calculate at least one output.
          */
         lookbackTotal = optInTimePeriod - 1;
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
            return RetCode.InsufficientHistory ;
         }
         /* TRIMA Description
          * =================
          * The triangular MA is a weighted moving average. Instead of the
          * TA_WMA who put more weigth on the latest price bar, the triangular
          * put more weigth on the data in the middle of the specified period.
          *
          * Examples:
          *   For TimeSerie={a,b,c,d,e,f...} ('a' is the older price)
          *
          *   1st value for TRIMA 4-Period is:  ((1*a)+(2*b)+(2*c)+(1*d)) / 6
          *   2nd value for TRIMA 4-Period is:  ((1*b)+(2*c)+(2*d)+(1*e)) / 6
          *
          *   1st value for TRIMA 5-Period is:  ((1*a)+(2*b)+(3*c)+(2*d)+(1*e)) / 9
          *   2nd value for TRIMA 5-Period is:  ((1*b)+(2*c)+(3*d)+(2*e)+(1*f)) / 9
          *
          * Generally Accepted Implementation
          * ==================================
          * Using algebra, it can be demonstrated that the TRIMA is equivalent to
          * doing a SMA of a SMA. The following explain the rules:
          *
          *  (1) When the period is even, TRIMA(x,period)=SMA(SMA(x,period/2),(period/2)+1)
          *  (2) When the period is odd,  TRIMA(x,period)=SMA(SMA(x,(period+1)/2),(period+1)/2)
          *
          * In other word:
          *  (1) A period of 4 becomes TRIMA(x,4) = SMA( SMA( x, 2), 3 )
          *  (2) A period of 5 becomes TRIMA(x,5) = SMA( SMA( x, 3), 3 )
          *
          * The SMA of a SMA is the algorithm generaly found in books.
          *
          * Tradestation Implementation
          * ===========================
          * Tradestation deviate from the generally accepted implementation by
          * making the TRIMA to be as follow:
          *    TRIMA(x,period) = SMA( SMA( x, (int)(period/2)+1), (int)(period/2)+1 );
          * This formula is done regardless if the period is even or odd.
          *
          * In other word:
          *  (1) A period of 4 becomes TRIMA(x,4) = SMA( SMA( x, 3), 3 )
          *  (2) A period of 5 becomes TRIMA(x,5) = SMA( SMA( x, 3), 3 )
          *  (3) A period of 6 becomes TRIMA(x,5) = SMA( SMA( x, 4), 4 )
          *  (4) A period of 7 becomes TRIMA(x,5) = SMA( SMA( x, 4), 4 )
          *
          * It is not clear to me if the Tradestation approach is a bug or a deliberate
          * decision to do things differently.
          *
          * Metastock Implementation
          * ========================
          * Output is the same as the generally accepted implementation.
          *
          * TA-Lib Implementation
          * =====================
          * Output is also the same as the generally accepted implementation.
          *
          * For speed optimization and avoid memory allocation, TA-Lib use
          * a better algorithm than the usual SMA of a SMA.
          *
          * The calculation from one TRIMA value to the next is done by doing 4
          * little adjustment (the following show a TRIMA 4-period):
          *
          * TRIMA at time 'd': ((1*a)+(2*b)+(2*c)+(1*d)) / 6
          * TRIMA at time 'e': ((1*b)+(2*c)+(2*d)+(1*e)) / 6
          *
          * To go from TRIMA 'd' to 'e', the following is done:
          *       1) 'a' and 'b' are substract from the numerator.
          *       2) 'd' is added to the numerator.
          *       3) 'e' is added to the numerator.
          *       4) Calculate TRIMA by doing numerator / 6
          *       5) Repeat sequence for next output
          *
          * These operations are the same steps done by TA-LIB:
          *       1) is done by numeratorSub
          *       2) is done by numeratorAdd.
          *       3) is obtain from the latest input
          *       4) Calculate and write TRIMA in the output
          *       5) Repeat for next output.
          *
          * Of course, numerotrAdd and numeratorSub needs to be
          * adjusted for each iteration.
          *
          * The update of numeratorSub needs values from the input at
          * the trailingIdx and middleIdx position.
          *
          * The update of numeratorAdd needs values from the input at
          * the middleIdx and todayIdx.
          */
         outIdx = 0;
         /* Logic for Odd period */
         /* Calculate the factor which is 1 divided by the
          * sumation of the weight.
          *
          * The sum of the weight is calculated as follow:
          *
          * The simple sumation serie 1+2+3... n can be
          * express as n(n+1)/2
          *
          * From this logic, a "triangular" sumation formula
          * can be found depending if the period is odd or even.
          *
          * Odd Period Formula:
          *  period = 5 and with n=(int)(period/2)
          *  the formula for a "triangular" serie is:
          *    1+2+3+2+1 = (n*(n+1))+n+1
          *              = (n+1)*(n+1)
          *              = 3 * 3 = 9
          *
          * Even period Formula:
          *   period = 6 and with n=(int)(period/2)
          *   the formula for a "triangular" serie is:
          *    1+2+3+3+2+1 = n*(n+1)
          *                = 3 * 4 = 12
          */
         /* Note: the (i+1) factors are widened to double so the product
          *       cannot overflow a 32-bit int at extreme periods (i+1 reaches
          *       ~50000 near the API maximum, and (i+1)*(i+1) exceeds INT_MAX
          *       past period ~92682). For every period where the int product
          *       fits, the widened value is identical.
          */
         i = optInTimePeriod >> 1;
         factor = (double)(i + 1) * (i + 1);
         factor = 1.0 / factor;
         /* Initialize all the variable before
          * starting to iterate for each output.
          */
         trailingIdx = startIdx - lookbackTotal;
         middleIdx = trailingIdx + i;
         todayIdx = middleIdx + i;
         numerator = 0.0;
         numeratorSub = 0.0;
         for( i = middleIdx; i >= trailingIdx; i -= 1 ) {
            tempReal = inReal[i];
            numeratorSub += tempReal;
            numerator += numeratorSub;
         }
         numeratorAdd = 0.0;
         middleIdx += 1;
         for( i = middleIdx; i <= todayIdx; i += 1 ) {
            tempReal = inReal[i];
            numeratorAdd += tempReal;
            numerator += numeratorAdd;
         }
         /* Write the first output */
         outIdx = 0;
         tempReal = inReal[trailingIdx++];
         outReal[outIdx++ * outStride] = numerator * factor;
         todayIdx += 1;
         /* Note: The value at the trailingIdx was saved
          *       in tempReal to account for the case where
          *       outReal and inReal are ptr on the same
          *       buffer.
          */
         /* Iterate for remaining output */
         while( todayIdx <= endIdx ) {
            /* Step (1) */
            numerator -= numeratorSub;
            numeratorSub -= tempReal;
            tempReal = inReal[middleIdx++];
            numeratorSub += tempReal;
            /* Step (2) */
            numerator += numeratorAdd;
            numeratorAdd -= tempReal;
            tempReal = inReal[todayIdx++];
            numeratorAdd += tempReal;
            /* Step (3) */
            numerator += tempReal;
            /* Step (4) */
            tempReal = inReal[trailingIdx++];
            outReal[outIdx++ * outStride] = numerator * factor;
         }
         outNBElement.value = outIdx;
         outBegIdx.value = startIdx;
         /* Capture the live batch state into the handle. */
         int cap_middleIdx = todayIdx - middleIdx;
         if( cap_middleIdx < 0 || cap_middleIdx > historyLen ) {
            return RetCode.InternalError;
         }
         int allocN_middleIdx = (cap_middleIdx > 0)? cap_middleIdx : 1;
         double[] capRing_middleIdx_inReal = new double[allocN_middleIdx];
         System.arraycopy(inReal, historyLen - cap_middleIdx, capRing_middleIdx_inReal, 0, cap_middleIdx);
         int cap_trailingIdx = todayIdx - trailingIdx;
         if( cap_trailingIdx < 0 || cap_trailingIdx > historyLen ) {
            return RetCode.InternalError;
         }
         int allocN_trailingIdx = (cap_trailingIdx > 0)? cap_trailingIdx : 1;
         double[] capRing_trailingIdx_inReal = new double[allocN_trailingIdx];
         System.arraycopy(inReal, historyLen - cap_trailingIdx, capRing_trailingIdx_inReal, 0, cap_trailingIdx);
         sp.optInTimePeriod = optInTimePeriod;
         sp.numerator = numerator;
         sp.numeratorSub = numeratorSub;
         sp.numeratorAdd = numeratorAdd;
         sp.factor = factor;
         sp.tempReal = tempReal;
         sp.ringPos_middleIdx = 0;
         sp.ringCap_middleIdx = cap_middleIdx;
         sp.ring_middleIdx_inReal = capRing_middleIdx_inReal;
         sp.ringPos_trailingIdx = 0;
         sp.ringCap_trailingIdx = cap_trailingIdx;
         sp.ring_trailingIdx_inReal = capRing_trailingIdx_inReal;
         sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
         return RetCode.Success;
      } else {
         int lookbackTotal = 0;
         double numerator = 0;
         double numeratorSub = 0;
         double numeratorAdd = 0;
         int i = 0;
         int outIdx = 0;
         int todayIdx = 0;
         int trailingIdx = 0;
         int middleIdx = 0;
         double factor = 0;
         double tempReal = 0;
         /* Identify the minimum number of price bar needed
          * to calculate at least one output.
          */
         lookbackTotal = optInTimePeriod - 1;
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
            return RetCode.InsufficientHistory ;
         }
         /* TRIMA Description
          * =================
          * The triangular MA is a weighted moving average. Instead of the
          * TA_WMA who put more weigth on the latest price bar, the triangular
          * put more weigth on the data in the middle of the specified period.
          *
          * Examples:
          *   For TimeSerie={a,b,c,d,e,f...} ('a' is the older price)
          *
          *   1st value for TRIMA 4-Period is:  ((1*a)+(2*b)+(2*c)+(1*d)) / 6
          *   2nd value for TRIMA 4-Period is:  ((1*b)+(2*c)+(2*d)+(1*e)) / 6
          *
          *   1st value for TRIMA 5-Period is:  ((1*a)+(2*b)+(3*c)+(2*d)+(1*e)) / 9
          *   2nd value for TRIMA 5-Period is:  ((1*b)+(2*c)+(3*d)+(2*e)+(1*f)) / 9
          *
          * Generally Accepted Implementation
          * ==================================
          * Using algebra, it can be demonstrated that the TRIMA is equivalent to
          * doing a SMA of a SMA. The following explain the rules:
          *
          *  (1) When the period is even, TRIMA(x,period)=SMA(SMA(x,period/2),(period/2)+1)
          *  (2) When the period is odd,  TRIMA(x,period)=SMA(SMA(x,(period+1)/2),(period+1)/2)
          *
          * In other word:
          *  (1) A period of 4 becomes TRIMA(x,4) = SMA( SMA( x, 2), 3 )
          *  (2) A period of 5 becomes TRIMA(x,5) = SMA( SMA( x, 3), 3 )
          *
          * The SMA of a SMA is the algorithm generaly found in books.
          *
          * Tradestation Implementation
          * ===========================
          * Tradestation deviate from the generally accepted implementation by
          * making the TRIMA to be as follow:
          *    TRIMA(x,period) = SMA( SMA( x, (int)(period/2)+1), (int)(period/2)+1 );
          * This formula is done regardless if the period is even or odd.
          *
          * In other word:
          *  (1) A period of 4 becomes TRIMA(x,4) = SMA( SMA( x, 3), 3 )
          *  (2) A period of 5 becomes TRIMA(x,5) = SMA( SMA( x, 3), 3 )
          *  (3) A period of 6 becomes TRIMA(x,5) = SMA( SMA( x, 4), 4 )
          *  (4) A period of 7 becomes TRIMA(x,5) = SMA( SMA( x, 4), 4 )
          *
          * It is not clear to me if the Tradestation approach is a bug or a deliberate
          * decision to do things differently.
          *
          * Metastock Implementation
          * ========================
          * Output is the same as the generally accepted implementation.
          *
          * TA-Lib Implementation
          * =====================
          * Output is also the same as the generally accepted implementation.
          *
          * For speed optimization and avoid memory allocation, TA-Lib use
          * a better algorithm than the usual SMA of a SMA.
          *
          * The calculation from one TRIMA value to the next is done by doing 4
          * little adjustment (the following show a TRIMA 4-period):
          *
          * TRIMA at time 'd': ((1*a)+(2*b)+(2*c)+(1*d)) / 6
          * TRIMA at time 'e': ((1*b)+(2*c)+(2*d)+(1*e)) / 6
          *
          * To go from TRIMA 'd' to 'e', the following is done:
          *       1) 'a' and 'b' are substract from the numerator.
          *       2) 'd' is added to the numerator.
          *       3) 'e' is added to the numerator.
          *       4) Calculate TRIMA by doing numerator / 6
          *       5) Repeat sequence for next output
          *
          * These operations are the same steps done by TA-LIB:
          *       1) is done by numeratorSub
          *       2) is done by numeratorAdd.
          *       3) is obtain from the latest input
          *       4) Calculate and write TRIMA in the output
          *       5) Repeat for next output.
          *
          * Of course, numerotrAdd and numeratorSub needs to be
          * adjusted for each iteration.
          *
          * The update of numeratorSub needs values from the input at
          * the trailingIdx and middleIdx position.
          *
          * The update of numeratorAdd needs values from the input at
          * the middleIdx and todayIdx.
          */
         outIdx = 0;
         /* Even logic.
          *
          * Very similar to the odd logic, except:
          *  - calculation of the factor is different.
          *  - the coverage of the numeratorSub and numeratorAdd is
          *    slightly different.
          *  - Adjustment of numeratorAdd is different. See Step (2).
          */
         i = optInTimePeriod >> 1;
         factor = (double)i * (i + 1);
         /* widen: i*(i+1) overflows int past period ~92682 */
         factor = 1.0 / factor;
         /* Initialize all the variable before
          * starting to iterate for each output.
          */
         trailingIdx = startIdx - lookbackTotal;
         middleIdx = trailingIdx + i - 1;
         todayIdx = middleIdx + i;
         numerator = 0.0;
         numeratorSub = 0.0;
         for( i = middleIdx; i >= trailingIdx; i -= 1 ) {
            tempReal = inReal[i];
            numeratorSub += tempReal;
            numerator += numeratorSub;
         }
         numeratorAdd = 0.0;
         middleIdx += 1;
         for( i = middleIdx; i <= todayIdx; i += 1 ) {
            tempReal = inReal[i];
            numeratorAdd += tempReal;
            numerator += numeratorAdd;
         }
         /* Write the first output */
         outIdx = 0;
         tempReal = inReal[trailingIdx++];
         outReal[outIdx++ * outStride] = numerator * factor;
         todayIdx += 1;
         /* Note: The value at the trailingIdx was saved
          *       in tempReal to account for the case where
          *       outReal and inReal are ptr on the same
          *       buffer.
          */
         /* Iterate for remaining output */
         while( todayIdx <= endIdx ) {
            /* Step (1) */
            numerator -= numeratorSub;
            numeratorSub -= tempReal;
            tempReal = inReal[middleIdx++];
            numeratorSub += tempReal;
            /* Step (2) */
            numeratorAdd -= tempReal;
            numerator += numeratorAdd;
            tempReal = inReal[todayIdx++];
            numeratorAdd += tempReal;
            /* Step (3) */
            numerator += tempReal;
            /* Step (4) */
            tempReal = inReal[trailingIdx++];
            outReal[outIdx++ * outStride] = numerator * factor;
         }
         outNBElement.value = outIdx;
         outBegIdx.value = startIdx;
         /* Capture the live batch state into the handle. */
         int cap_middleIdx = todayIdx - middleIdx;
         if( cap_middleIdx < 0 || cap_middleIdx > historyLen ) {
            return RetCode.InternalError;
         }
         int allocN_middleIdx = (cap_middleIdx > 0)? cap_middleIdx : 1;
         double[] capRing_middleIdx_inReal = new double[allocN_middleIdx];
         System.arraycopy(inReal, historyLen - cap_middleIdx, capRing_middleIdx_inReal, 0, cap_middleIdx);
         int cap_trailingIdx = todayIdx - trailingIdx;
         if( cap_trailingIdx < 0 || cap_trailingIdx > historyLen ) {
            return RetCode.InternalError;
         }
         int allocN_trailingIdx = (cap_trailingIdx > 0)? cap_trailingIdx : 1;
         double[] capRing_trailingIdx_inReal = new double[allocN_trailingIdx];
         System.arraycopy(inReal, historyLen - cap_trailingIdx, capRing_trailingIdx_inReal, 0, cap_trailingIdx);
         sp.optInTimePeriod = optInTimePeriod;
         sp.numerator = numerator;
         sp.numeratorSub = numeratorSub;
         sp.numeratorAdd = numeratorAdd;
         sp.factor = factor;
         sp.tempReal = tempReal;
         sp.ringPos_middleIdx = 0;
         sp.ringCap_middleIdx = cap_middleIdx;
         sp.ring_middleIdx_inReal = capRing_middleIdx_inReal;
         sp.ringPos_trailingIdx = 0;
         sp.ringCap_trailingIdx = cap_trailingIdx;
         sp.ring_trailingIdx_inReal = capRing_trailingIdx_inReal;
         sp.cur_outReal = outReal[(outNBElement.value - 1) * outStride];
         return RetCode.Success;
      }
   }
   /* trimaOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   TrimaStream trimaOpenAndFillInternal( double inReal[], int startIdx, int optInTimePeriod, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      TrimaStream sp = new TrimaStream(this);
      RetCode retCode = trimaOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, outReal, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("TRIMA openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("TRIMA openAndFill: internal error", retCode);
      }
      throw new TaLibArgumentException("TRIMA openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind trimaOpen (composition seam). */
   TrimaStream trimaOpenInternal( double inReal[], int startIdx, int optInTimePeriod )
   {
      TrimaStream sp = new TrimaStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      RetCode retCode = trimaOpenImpl(sp, inReal, startIdx, optInTimePeriod, outBegIdx, outNBElement, sink_outReal, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.InsufficientHistory ) {
         throw new InsufficientHistoryException("TRIMA open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new TaLibStateException("TRIMA open: internal error", retCode);
      }
      throw new TaLibArgumentException("TRIMA open: " + retCode, retCode);
   }
   /**
    * Open a live TRIMA stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#TRIMA} at that bar.
    * <p>The history must hold at least {@code TRIMA_Lookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public TrimaStream trimaOpen( double inReal[], int optInTimePeriod )
   {
      requireArgument("TRIMA open", "inReal", inReal);
      requireHistory("TRIMA open", inReal.length);
      return trimaOpenInternal(inReal, 0, optInTimePeriod);
   }
   /**
    * {@link Core#trimaOpen} that also fills the output array(s) bit-identically
    * to {@link Core#TRIMA} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link TrimaStream#outRange()}.
    */
   public TrimaStream trimaOpenAndFill( double inReal[], int optInTimePeriod, double outReal[] )
   {
      requireArgument("TRIMA openAndFill", "inReal", inReal);
      requireHistory("TRIMA openAndFill", inReal.length);
      int guardOutLen = openFillCount("TRIMA openAndFill", inReal.length, TRIMA_Lookback(optInTimePeriod));
      requireLength("TRIMA openAndFill", "outReal", outReal, guardOutLen);
      if( (Object)outReal == (Object)inReal ) {
         throw new TaLibArgumentException("TRIMA openAndFill: " + RetCode.BadParam, RetCode.BadParam);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return trimaOpenAndFillInternal(inReal, 0, optInTimePeriod, outBegIdx, outNBElement, outReal);
   }
