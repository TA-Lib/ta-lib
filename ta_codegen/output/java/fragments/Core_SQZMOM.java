/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  KL       Kevin Lin (@kevinlincg)
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  100826 KL,CC  Creation (#488).
 */

   /**
    * Number of leading input bars {@link Core#sqzmom} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInBBPeriod Number of bars in the Bollinger Band window (default
    *        20; range 2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInNbDev Number of standard deviations the Bollinger Bands are
    *        placed at (default 2; minimum 0; {@link Core#REAL_DEFAULT} selects the
    *        default).
    * @param optInKCPeriod Number of bars in the Keltner Channel, the true-range
    *        mean and the regression (default 20; range 2..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @param optInFactorWide Keltner width, in true-range means, of the widest
    *        channel (default 2; minimum 0; {@link Core#REAL_DEFAULT} selects the
    *        default).
    * @param optInFactorNormal Keltner width of the classic channel (default
    *        1.5; minimum 0; {@link Core#REAL_DEFAULT} selects the default).
    * @param optInFactorNarrow Keltner width of the tightest channel (default 1;
    *        minimum 0; {@link Core#REAL_DEFAULT} selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int sqzmomLookback( int optInBBPeriod, double optInNbDev, int optInKCPeriod, double optInFactorWide, double optInFactorNormal, double optInFactorNarrow )
   {
      if( optInBBPeriod == Integer.MIN_VALUE ) {
         optInBBPeriod = 20;
      } else if( optInBBPeriod < 2 || optInBBPeriod > 100000 ) {
         return -1;
      }
      if( optInNbDev == REAL_DEFAULT ) {
         optInNbDev = 2e0;
      } else if( !(optInNbDev >= 0e0 && optInNbDev <= REAL_MAX) ) {
         return -1;
      }
      if( optInKCPeriod == Integer.MIN_VALUE ) {
         optInKCPeriod = 20;
      } else if( optInKCPeriod < 2 || optInKCPeriod > 100000 ) {
         return -1;
      }
      if( optInFactorWide == REAL_DEFAULT ) {
         optInFactorWide = 2e0;
      } else if( !(optInFactorWide >= 0e0 && optInFactorWide <= REAL_MAX) ) {
         return -1;
      }
      if( optInFactorNormal == REAL_DEFAULT ) {
         optInFactorNormal = 1.5e0;
      } else if( !(optInFactorNormal >= 0e0 && optInFactorNormal <= REAL_MAX) ) {
         return -1;
      }
      if( optInFactorNarrow == REAL_DEFAULT ) {
         optInFactorNarrow = 1e0;
      } else if( !(optInFactorNarrow >= 0e0 && optInFactorNarrow <= REAL_MAX) ) {
         return -1;
      }
      int lookbackTotal;
      int leg;
      /* Three chains reach the first bar at which both outputs exist, and the
       * first output is the longest of them:
       *
       *   bands        sma(b) / stddev(b)           = b-1
       *   compression  trange -> sma(k)             = 1 + (k-1) = k
       *   momentum     midprice(k) -> linearreg(k)  = (k-1) + (k-1) = 2(k-1)
       *
       * The Keltner centre, sma(k) on the close, is k-1 and never dominates the
       * momentum chain it also feeds. At b = k = 20 the three read 19, 20 and 38,
       * which is where pandas, ta4j and trading-signals put their first value.
       * The factors scale a band that already exists, so they do not enter.
       */
      lookbackTotal = smaLookback(optInBBPeriod);
      leg = stddevLookback(optInBBPeriod, 1.0);
      if( leg > lookbackTotal ) {
         lookbackTotal = leg;
      }
      leg = trangeLookback() + smaLookback(optInKCPeriod);
      if( leg > lookbackTotal ) {
         lookbackTotal = leg;
      }
      leg = midpriceLookback(optInKCPeriod) + linearregLookback(optInKCPeriod);
      if( leg > lookbackTotal ) {
         lookbackTotal = leg;
      }
      return lookbackTotal ;

   }
   /**
    * How many bars ahead (positive) or behind (negative) of the bar that
    * computed it a chart draws one output of {@link Core#sqzmom}.
    * <p>Every output of this function is drawn at its own bar, so the answer is
    * 0.
    *
    * @param optInBBPeriod Number of bars in the Bollinger Band window (default
    *        20; range 2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInNbDev Number of standard deviations the Bollinger Bands are
    *        placed at (default 2; minimum 0; {@link Core#REAL_DEFAULT} selects the
    *        default).
    * @param optInKCPeriod Number of bars in the Keltner Channel, the true-range
    *        mean and the regression (default 20; range 2..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @param optInFactorWide Keltner width, in true-range means, of the widest
    *        channel (default 2; minimum 0; {@link Core#REAL_DEFAULT} selects the
    *        default).
    * @param optInFactorNormal Keltner width of the classic channel (default
    *        1.5; minimum 0; {@link Core#REAL_DEFAULT} selects the default).
    * @param optInFactorNarrow Keltner width of the tightest channel (default 1;
    *        minimum 0; {@link Core#REAL_DEFAULT} selects the default).
    * @param outputIdx Position of the output in the batch signature, from 0.
    * @return The display shift, or {@code Integer.MIN_VALUE} if a parameter is
    *        out of range or the index names no output.
    */
   public int sqzmomDisplayShift( int optInBBPeriod, double optInNbDev, int optInKCPeriod, double optInFactorWide, double optInFactorNormal, double optInFactorNarrow, int outputIdx )
   {
      if( sqzmomLookback( optInBBPeriod, optInNbDev, optInKCPeriod, optInFactorWide, optInFactorNormal, optInFactorNarrow ) < 0 ) {
         return Integer.MIN_VALUE;
      }
      if( outputIdx < 0 || outputIdx >= 2 ) {
         return Integer.MIN_VALUE;
      }
      return 0;
   }
   RetCode sqzmomImpl( int startIdx,
                       int endIdx,
                       double inHigh[],
                       double inLow[],
                       double inClose[],
                       int optInBBPeriod,
                       double optInNbDev,
                       int optInKCPeriod,
                       double optInFactorWide,
                       double optInFactorNormal,
                       double optInFactorNarrow,
                       MInteger outBegIdx,
                       MInteger outNBElement,
                       double outMomentum[],
                       int outSqueeze[] )
   {
      RetCode retCode;
      int lookbackTotal = 0;
      int stageLookback = 0;
      int stageIdx = 0;
      int stageLen = 0;
      int i = 0;
      int n = 0;
      MInteger tempBegIdx = new MInteger();
      MInteger tempNbElement = new MInteger();
      double[] tempDev;
      double[] tempWork;
      double[] tempKCM;
      double[] tempMid;
      double[] tempSD;
      double up = 0;
      double lo = 0;
      double centre = 0;
      double band = 0;
      if( (startIdx < 0) || (startIdx > INDEX_MAX) ) {
         return RetCode.OUT_OF_RANGE_START_INDEX ;
      }
      if( (endIdx < 0) || (endIdx > INDEX_MAX) || (endIdx < startIdx)) {
         return RetCode.OUT_OF_RANGE_END_INDEX ;
      }
      if( optInBBPeriod == Integer.MIN_VALUE ) {
         optInBBPeriod = 20;
      } else if( optInBBPeriod < 2 || optInBBPeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInNbDev == REAL_DEFAULT ) {
         optInNbDev = 2e0;
      } else if( !(optInNbDev >= 0e0 && optInNbDev <= REAL_MAX) ) {
         return RetCode.BAD_PARAM;
      }
      if( optInKCPeriod == Integer.MIN_VALUE ) {
         optInKCPeriod = 20;
      } else if( optInKCPeriod < 2 || optInKCPeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInFactorWide == REAL_DEFAULT ) {
         optInFactorWide = 2e0;
      } else if( !(optInFactorWide >= 0e0 && optInFactorWide <= REAL_MAX) ) {
         return RetCode.BAD_PARAM;
      }
      if( optInFactorNormal == REAL_DEFAULT ) {
         optInFactorNormal = 1.5e0;
      } else if( !(optInFactorNormal >= 0e0 && optInFactorNormal <= REAL_MAX) ) {
         return RetCode.BAD_PARAM;
      }
      if( optInFactorNarrow == REAL_DEFAULT ) {
         optInFactorNarrow = 1e0;
      } else if( !(optInFactorNarrow >= 0e0 && optInFactorNarrow <= REAL_MAX) ) {
         return RetCode.BAD_PARAM;
      }
      /* Every leg is a shipped function, CALLED rather than transcribed, so both
       * outputs carry each callee's own arithmetic bit for bit:
       *
       *   BBU, BBL = sma(close, b) +/- nbdev * stddev(close, b, 1.0)
       *   KCM      = sma(close, k)
       *   BAND     = sma(trange(high,low,close), k)      the simple mean of TR
       *   DMID     = midprice(high, low, k)
       *   DEV      = close - (DMID + KCM)/2
       *   momentum = linearreg(DEV, k)
       *
       * The bands are taken as the MA + STDDEV pair rather than through bbands
       * because bbands' SMA fast path states it is bit-identical to that pair, and
       * it is what bbands' own stream tier composes; taking the pair here keeps
       * every optional argument a parameter or a literal.
       *
       * DEV is built from shipped functions too, not in a loop of its own:
       * medprice is (a+b)/2 over any two series, and sma at a period of 1 is the
       * identity -- a one-element sum divided by 1.0, exact for every double --
       * which is what puts the close into the staged range's own indexing so that
       * sub can take the difference.
       *
       * Two chains need their input one window earlier than the first output bar:
       * sma(k) over TR, and linearreg(k) over DEV. Both windows are k-1 long, so
       * one staged range serves both.
       */
      lookbackTotal = sqzmomLookback(optInBBPeriod, optInNbDev, optInKCPeriod, optInFactorWide, optInFactorNormal, optInFactorNarrow);
      if( lookbackTotal > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.SUCCESS ;
      }
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.SUCCESS ;
      }
      n = endIdx - startIdx + 1;
      stageLookback = linearregLookback(optInKCPeriod);
      stageIdx = startIdx - stageLookback;
      stageLen = n + stageLookback;
      tempDev = new double[(int)(stageLen * 1)];
      tempWork = new double[(int)(stageLen * 1)];
      tempKCM = new double[(int)(n * 1)];
      tempMid = new double[(int)(n * 1)];
      tempSD = new double[(int)(n * 1)];
      /* The momentum anchor: the Donchian midpoint and the Keltner centre, averaged.
       * midprice is bit-identical to donchian's middle band.
       */
      OutRange _xr0 = midprice(stageIdx, endIdx, inHigh, inLow, optInKCPeriod, tempDev);
      tempBegIdx.value = _xr0.begIdx();
      tempNbElement.value = _xr0.count();
      retCode = RetCode.SUCCESS;
      OutRange _xr1 = sma(stageIdx, endIdx, inClose, optInKCPeriod, tempWork);
      tempBegIdx.value = _xr1.begIdx();
      tempNbElement.value = _xr1.count();
      retCode = RetCode.SUCCESS;
      /* tempDev becomes (DMID + KCM)/2 in place: medprice reads and writes the same
       * index, so the overwrite is safe.
       */
      OutRange _xr2 = medprice(0, stageLen - 1, tempDev, tempWork, tempDev);
      tempBegIdx.value = _xr2.begIdx();
      tempNbElement.value = _xr2.count();
      retCode = RetCode.SUCCESS;
      /* The close over the staged range, in that range's own indexing. */
      OutRange _xr3 = sma(stageIdx, endIdx, inClose, 1, tempWork);
      tempBegIdx.value = _xr3.begIdx();
      tempNbElement.value = _xr3.count();
      retCode = RetCode.SUCCESS;
      OutRange _xr4 = sub(0, stageLen - 1, tempWork, tempDev, tempDev);
      tempBegIdx.value = _xr4.begIdx();
      tempNbElement.value = _xr4.count();
      retCode = RetCode.SUCCESS;
      /* The compression band: the simple mean of the true range, not an ATR.
       * trange's own lookback is 1, and stageIdx is at least 1 because the
       * compression chain alone puts lookbackTotal at k or more.
       */
      OutRange _xr5 = trange(stageIdx, endIdx, inHigh, inLow, inClose, tempWork);
      tempBegIdx.value = _xr5.begIdx();
      tempNbElement.value = _xr5.count();
      retCode = RetCode.SUCCESS;
      OutRange _xr6 = sma(0, stageLen - 1, tempWork, optInKCPeriod, tempWork);
      tempBegIdx.value = _xr6.begIdx();
      tempNbElement.value = _xr6.count();
      retCode = RetCode.SUCCESS;
      OutRange _xr7 = sma(startIdx, endIdx, inClose, optInBBPeriod, tempMid);
      tempBegIdx.value = _xr7.begIdx();
      tempNbElement.value = _xr7.count();
      retCode = RetCode.SUCCESS;
      OutRange _xr8 = stddev(startIdx, endIdx, inClose, optInBBPeriod, 1.0, tempSD);
      tempBegIdx.value = _xr8.begIdx();
      tempNbElement.value = _xr8.count();
      retCode = RetCode.SUCCESS;
      OutRange _xr9 = sma(startIdx, endIdx, inClose, optInKCPeriod, tempKCM);
      tempBegIdx.value = _xr9.begIdx();
      tempNbElement.value = _xr9.count();
      retCode = RetCode.SUCCESS;
      /* LAST of the calls, because it is the first to write a caller buffer. Every
       * read of high, low and close is above it, so an output aliased onto one of
       * them -- which the abstract layer tests on purpose -- is still intact when it
       * is read.
       *
       * The regression window closes on the first output bar, so entering it at 0 of
       * the staged range puts its first fitted value on startIdx.
       */
      OutRange _xr10 = linearreg(0, stageLen - 1, tempDev, optInKCPeriod, outMomentum);
      tempBegIdx.value = _xr10.begIdx();
      tempNbElement.value = _xr10.count();
      retCode = RetCode.SUCCESS;
      /* The ordinal. The three Keltner widths share the centre and BAND >= 0, so
       * the tests nest: inside(narrow) implies inside(normal) implies inside(wide).
       * Testing narrow first is what makes the ordinal monotone without comparing
       * the factors to each other, which is why the card asks for
       * `wide >= normal >= narrow >= 0` and not for a strict order.
       */
      for( i = 0; i < n; i += 1 ) {
         up = Math.fma(tempSD[i], optInNbDev, tempMid[i]);
         lo = tempMid[i] - tempSD[i] * optInNbDev;
         centre = tempKCM[i];
         band = tempWork[i];
         if( lo > centre - optInFactorNarrow * band && up < centre + optInFactorNarrow * band ) {
            outSqueeze[i] = 3;
         } else if( lo > centre - optInFactorNormal * band && up < centre + optInFactorNormal * band ) {
            outSqueeze[i] = 2;
         } else if( lo > centre - optInFactorWide * band && up < centre + optInFactorWide * band ) {
            outSqueeze[i] = 1;
         } else if( lo < centre - optInFactorWide * band && up > centre + optInFactorWide * band ) {
            outSqueeze[i] = -1;
         } else {
            outSqueeze[i] = 0;
         }
      }
      outBegIdx.value = startIdx;
      outNBElement.value = n;
      return RetCode.SUCCESS ;
   }
   RetCode sqzmomImpl( int startIdx,
                       int endIdx,
                       float inHigh[],
                       float inLow[],
                       float inClose[],
                       int optInBBPeriod,
                       double optInNbDev,
                       int optInKCPeriod,
                       double optInFactorWide,
                       double optInFactorNormal,
                       double optInFactorNarrow,
                       MInteger outBegIdx,
                       MInteger outNBElement,
                       double outMomentum[],
                       int outSqueeze[] )
   {
      RetCode retCode;
      int lookbackTotal = 0;
      int stageLookback = 0;
      int stageIdx = 0;
      int stageLen = 0;
      int i = 0;
      int n = 0;
      MInteger tempBegIdx = new MInteger();
      MInteger tempNbElement = new MInteger();
      double[] tempDev;
      double[] tempWork;
      double[] tempKCM;
      double[] tempMid;
      double[] tempSD;
      double up = 0;
      double lo = 0;
      double centre = 0;
      double band = 0;
      if( (startIdx < 0) || (startIdx > INDEX_MAX) ) {
         return RetCode.OUT_OF_RANGE_START_INDEX ;
      }
      if( (endIdx < 0) || (endIdx > INDEX_MAX) || (endIdx < startIdx)) {
         return RetCode.OUT_OF_RANGE_END_INDEX ;
      }
      if( optInBBPeriod == Integer.MIN_VALUE ) {
         optInBBPeriod = 20;
      } else if( optInBBPeriod < 2 || optInBBPeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInNbDev == REAL_DEFAULT ) {
         optInNbDev = 2e0;
      } else if( !(optInNbDev >= 0e0 && optInNbDev <= REAL_MAX) ) {
         return RetCode.BAD_PARAM;
      }
      if( optInKCPeriod == Integer.MIN_VALUE ) {
         optInKCPeriod = 20;
      } else if( optInKCPeriod < 2 || optInKCPeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInFactorWide == REAL_DEFAULT ) {
         optInFactorWide = 2e0;
      } else if( !(optInFactorWide >= 0e0 && optInFactorWide <= REAL_MAX) ) {
         return RetCode.BAD_PARAM;
      }
      if( optInFactorNormal == REAL_DEFAULT ) {
         optInFactorNormal = 1.5e0;
      } else if( !(optInFactorNormal >= 0e0 && optInFactorNormal <= REAL_MAX) ) {
         return RetCode.BAD_PARAM;
      }
      if( optInFactorNarrow == REAL_DEFAULT ) {
         optInFactorNarrow = 1e0;
      } else if( !(optInFactorNarrow >= 0e0 && optInFactorNarrow <= REAL_MAX) ) {
         return RetCode.BAD_PARAM;
      }
      lookbackTotal = sqzmomLookback(optInBBPeriod, optInNbDev, optInKCPeriod, optInFactorWide, optInFactorNormal, optInFactorNarrow);
      if( lookbackTotal > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.SUCCESS ;
      }
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.SUCCESS ;
      }
      n = endIdx - startIdx + 1;
      stageLookback = linearregLookback(optInKCPeriod);
      stageIdx = startIdx - stageLookback;
      stageLen = n + stageLookback;
      tempDev = new double[(int)(stageLen * 1)];
      tempWork = new double[(int)(stageLen * 1)];
      tempKCM = new double[(int)(n * 1)];
      tempMid = new double[(int)(n * 1)];
      tempSD = new double[(int)(n * 1)];
      OutRange _xr0 = midprice(stageIdx, endIdx, inHigh, inLow, optInKCPeriod, tempDev);
      tempBegIdx.value = _xr0.begIdx();
      tempNbElement.value = _xr0.count();
      retCode = RetCode.SUCCESS;
      OutRange _xr1 = sma(stageIdx, endIdx, inClose, optInKCPeriod, tempWork);
      tempBegIdx.value = _xr1.begIdx();
      tempNbElement.value = _xr1.count();
      retCode = RetCode.SUCCESS;
      OutRange _xr2 = medprice(0, stageLen - 1, tempDev, tempWork, tempDev);
      tempBegIdx.value = _xr2.begIdx();
      tempNbElement.value = _xr2.count();
      retCode = RetCode.SUCCESS;
      OutRange _xr3 = sma(stageIdx, endIdx, inClose, 1, tempWork);
      tempBegIdx.value = _xr3.begIdx();
      tempNbElement.value = _xr3.count();
      retCode = RetCode.SUCCESS;
      OutRange _xr4 = sub(0, stageLen - 1, tempWork, tempDev, tempDev);
      tempBegIdx.value = _xr4.begIdx();
      tempNbElement.value = _xr4.count();
      retCode = RetCode.SUCCESS;
      OutRange _xr5 = trange(stageIdx, endIdx, inHigh, inLow, inClose, tempWork);
      tempBegIdx.value = _xr5.begIdx();
      tempNbElement.value = _xr5.count();
      retCode = RetCode.SUCCESS;
      OutRange _xr6 = sma(0, stageLen - 1, tempWork, optInKCPeriod, tempWork);
      tempBegIdx.value = _xr6.begIdx();
      tempNbElement.value = _xr6.count();
      retCode = RetCode.SUCCESS;
      OutRange _xr7 = sma(startIdx, endIdx, inClose, optInBBPeriod, tempMid);
      tempBegIdx.value = _xr7.begIdx();
      tempNbElement.value = _xr7.count();
      retCode = RetCode.SUCCESS;
      OutRange _xr8 = stddev(startIdx, endIdx, inClose, optInBBPeriod, 1.0, tempSD);
      tempBegIdx.value = _xr8.begIdx();
      tempNbElement.value = _xr8.count();
      retCode = RetCode.SUCCESS;
      OutRange _xr9 = sma(startIdx, endIdx, inClose, optInKCPeriod, tempKCM);
      tempBegIdx.value = _xr9.begIdx();
      tempNbElement.value = _xr9.count();
      retCode = RetCode.SUCCESS;
      OutRange _xr10 = linearreg(0, stageLen - 1, tempDev, optInKCPeriod, outMomentum);
      tempBegIdx.value = _xr10.begIdx();
      tempNbElement.value = _xr10.count();
      retCode = RetCode.SUCCESS;
      for( i = 0; i < n; i += 1 ) {
         up = Math.fma(tempSD[i], optInNbDev, tempMid[i]);
         lo = tempMid[i] - tempSD[i] * optInNbDev;
         centre = tempKCM[i];
         band = tempWork[i];
         if( lo > centre - optInFactorNarrow * band && up < centre + optInFactorNarrow * band ) {
            outSqueeze[i] = 3;
         } else if( lo > centre - optInFactorNormal * band && up < centre + optInFactorNormal * band ) {
            outSqueeze[i] = 2;
         } else if( lo > centre - optInFactorWide * band && up < centre + optInFactorWide * band ) {
            outSqueeze[i] = 1;
         } else if( lo < centre - optInFactorWide * band && up > centre + optInFactorWide * band ) {
            outSqueeze[i] = -1;
         } else {
            outSqueeze[i] = 0;
         }
      }
      outBegIdx.value = startIdx;
      outNBElement.value = n;
      return RetCode.SUCCESS ;
   }
   /**
    * John Carter's TTM Squeeze, in the form LazyBear published on TradingView
    * in 2014. Volatility compression is read by comparing the Bollinger Bands
    * against a Keltner Channel: when the bands sit inside the channel the
    * market is coiled, and when they push outside it the coil has released.
    * LazyBear replaced Carter's simple momentum with a linear regression of the
    * close against a Donchian/SMA anchor, which is the histogram that
    * StockCharts, pandas, ta4j and trading-signals all compute. "Squeeze Pro"
    * tests the bands against three Keltner widths instead of one, so
    * compression becomes a level rather than a boolean. {@code outSqueeze}
    * carries that level: 3, 2 and 1 for the narrow, normal and wide channels,
    * -1 once the bands are outside the wide channel, and 0 in between. The
    * classic squeeze-on flag is {@code outSqueeze &gt;= 2}, and with
    * {@code wide == normal} the three states are exactly LazyBear's on / off /
    * none.
    * <p>Formula and more info at <a
    * href="https://ta-lib.org/functions/sqzmom">ta-lib.org/functions/sqzmom</a>.
    * <p><b>Notes</b>
    * <ul>
    * <li>{@code BAND} is the simple mean of the true range over {@code k} bars, not an ATR: no Wilder smoothing, and so no unstable period anywhere in this function.</li>
    * <li>The anchor is written three ways across the sources — {@code avg(avg(HH,LL), SMA)}, {@code ((HH+LL)/2 + SMA)/2} and {@code 0.25*(HH+LL) + 0.5*SMA}. All three give the same double, because scaling by a power of two is exact.</li>
    * <li>The three channels share a centre and {@code BAND &gt;= 0}, so they nest: inside the narrow one implies inside the normal one implies inside the wide one. The ordinal is therefore monotone, and the factors are required only to be ordered {@code wide &gt;= normal &gt;= narrow &gt;= 0}, not strictly.</li>
    * <li>The momentum window is the Keltner period, as in LazyBear's script, pandas and trading-signals.</li>
    * <li>The three chains that feed the outputs do not reach their first value together. The first output bar is the latest of them, so where {@code b} is much larger than {@code k} the momentum exists before the state does and is still withheld until both do. pandas emits the earlier momentum; a capture compared against it must start at the later bar.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are, and the
    * library never pads with NaN. A valid range that ends before
    * {@link Core#sqzmomLookback} is a <b>success with no values</b>
    * ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Closing price of each bar.
    * @param optInBBPeriod Number of bars in the Bollinger Band window (default
    *        20; range 2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInNbDev Number of standard deviations the Bollinger Bands are
    *        placed at (default 2; minimum 0; {@link Core#REAL_DEFAULT} selects the
    *        default).
    * @param optInKCPeriod Number of bars in the Keltner Channel, the true-range
    *        mean and the regression (default 20; range 2..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @param optInFactorWide Keltner width, in true-range means, of the widest
    *        channel (default 2; minimum 0; {@link Core#REAL_DEFAULT} selects the
    *        default).
    * @param optInFactorNormal Keltner width of the classic channel (default
    *        1.5; minimum 0; {@link Core#REAL_DEFAULT} selects the default).
    * @param optInFactorNarrow Keltner width of the tightest channel (default 1;
    *        minimum 0; {@link Core#REAL_DEFAULT} selects the default).
    * @param outMomentum Linear regression of the close against the Donchian/SMA
    *        anchor. Must hold at least
    *        {@code endIdx - max(startIdx, sqzmomLookback(...)) + 1} values, and never
    *        be empty: an empty array is an absent output.
    * @param outSqueeze Compression level: 3 narrow, 2 normal, 1 wide, -1
    *        released, 0 otherwise. Must hold at least
    *        {@code endIdx - max(startIdx, sqzmomLookback(...)) + 1} values, and never
    *        be empty: an empty array is an absent output.
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
    * @see Core#bbands
    * @see Core#kc
    * @see Core#trange
    * @see Core#midprice
    * @see Core#linearreg
    */
   public OutRange sqzmom( int startIdx,
                           int endIdx,
                           double inHigh[],
                           double inLow[],
                           double inClose[],
                           int optInBBPeriod,
                           double optInNbDev,
                           int optInKCPeriod,
                           double optInFactorWide,
                           double optInFactorNormal,
                           double optInFactorNarrow,
                           double outMomentum[],
                           int outSqueeze[] )
   {
      requireIndexRange("SQZMOM", startIdx, endIdx);
      int guardStart = clampedStart("SQZMOM", startIdx, sqzmomLookback(optInBBPeriod, optInNbDev, optInKCPeriod, optInFactorWide, optInFactorNormal, optInFactorNarrow));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("SQZMOM", "inHigh", inHigh, guardInLen);
      requireLength("SQZMOM", "inLow", inLow, guardInLen);
      requireLength("SQZMOM", "inClose", inClose, guardInLen);
      requireLength("SQZMOM", "outMomentum", outMomentum, guardOutLen);
      requireLength("SQZMOM", "outSqueeze", outSqueeze, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = sqzmomImpl(startIdx, endIdx, inHigh, inLow, inClose, optInBBPeriod, optInNbDev, optInKCPeriod, optInFactorWide, optInFactorNormal, optInFactorNarrow, outBegIdx, outNBElement, outMomentum, outSqueeze);
      if( retCode != RetCode.SUCCESS ) {
         throw failure("SQZMOM", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * John Carter's TTM Squeeze, in the form LazyBear published on TradingView
    * in 2014. Volatility compression is read by comparing the Bollinger Bands
    * against a Keltner Channel: when the bands sit inside the channel the
    * market is coiled, and when they push outside it the coil has released.
    * LazyBear replaced Carter's simple momentum with a linear regression of the
    * close against a Donchian/SMA anchor, which is the histogram that
    * StockCharts, pandas, ta4j and trading-signals all compute. "Squeeze Pro"
    * tests the bands against three Keltner widths instead of one, so
    * compression becomes a level rather than a boolean. {@code outSqueeze}
    * carries that level: 3, 2 and 1 for the narrow, normal and wide channels,
    * -1 once the bands are outside the wide channel, and 0 in between. The
    * classic squeeze-on flag is {@code outSqueeze &gt;= 2}, and with
    * {@code wide == normal} the three states are exactly LazyBear's on / off /
    * none.
    * <p>Formula and more info at <a
    * href="https://ta-lib.org/functions/sqzmom">ta-lib.org/functions/sqzmom</a>.
    * <p><b>Notes</b>
    * <ul>
    * <li>{@code BAND} is the simple mean of the true range over {@code k} bars, not an ATR: no Wilder smoothing, and so no unstable period anywhere in this function.</li>
    * <li>The anchor is written three ways across the sources — {@code avg(avg(HH,LL), SMA)}, {@code ((HH+LL)/2 + SMA)/2} and {@code 0.25*(HH+LL) + 0.5*SMA}. All three give the same double, because scaling by a power of two is exact.</li>
    * <li>The three channels share a centre and {@code BAND &gt;= 0}, so they nest: inside the narrow one implies inside the normal one implies inside the wide one. The ordinal is therefore monotone, and the factors are required only to be ordered {@code wide &gt;= normal &gt;= narrow &gt;= 0}, not strictly.</li>
    * <li>The momentum window is the Keltner period, as in LazyBear's script, pandas and trading-signals.</li>
    * <li>The three chains that feed the outputs do not reach their first value together. The first output bar is the latest of them, so where {@code b} is much larger than {@code k} the momentum exists before the state does and is still withheld until both do. pandas emits the earlier momentum; a capture compared against it must start at the later bar.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are, and the
    * library never pads with NaN. A valid range that ends before
    * {@link Core#sqzmomLookback} is a <b>success with no values</b>
    * ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inHigh High price of each bar.
    * @param inLow Low price of each bar.
    * @param inClose Closing price of each bar.
    * @param optInBBPeriod Number of bars in the Bollinger Band window (default
    *        20; range 2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInNbDev Number of standard deviations the Bollinger Bands are
    *        placed at (default 2; minimum 0; {@link Core#REAL_DEFAULT} selects the
    *        default).
    * @param optInKCPeriod Number of bars in the Keltner Channel, the true-range
    *        mean and the regression (default 20; range 2..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @param optInFactorWide Keltner width, in true-range means, of the widest
    *        channel (default 2; minimum 0; {@link Core#REAL_DEFAULT} selects the
    *        default).
    * @param optInFactorNormal Keltner width of the classic channel (default
    *        1.5; minimum 0; {@link Core#REAL_DEFAULT} selects the default).
    * @param optInFactorNarrow Keltner width of the tightest channel (default 1;
    *        minimum 0; {@link Core#REAL_DEFAULT} selects the default).
    * @param outMomentum Linear regression of the close against the Donchian/SMA
    *        anchor. Must hold at least
    *        {@code endIdx - max(startIdx, sqzmomLookback(...)) + 1} values, and never
    *        be empty: an empty array is an absent output.
    * @param outSqueeze Compression level: 3 narrow, 2 normal, 1 wide, -1
    *        released, 0 otherwise. Must hold at least
    *        {@code endIdx - max(startIdx, sqzmomLookback(...)) + 1} values, and never
    *        be empty: an empty array is an absent output.
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
    * @see Core#bbands
    * @see Core#kc
    * @see Core#trange
    * @see Core#midprice
    * @see Core#linearreg
    */
   public OutRange sqzmom( int startIdx,
                           int endIdx,
                           float inHigh[],
                           float inLow[],
                           float inClose[],
                           int optInBBPeriod,
                           double optInNbDev,
                           int optInKCPeriod,
                           double optInFactorWide,
                           double optInFactorNormal,
                           double optInFactorNarrow,
                           double outMomentum[],
                           int outSqueeze[] )
   {
      requireIndexRange("SQZMOM", startIdx, endIdx);
      int guardStart = clampedStart("SQZMOM", startIdx, sqzmomLookback(optInBBPeriod, optInNbDev, optInKCPeriod, optInFactorWide, optInFactorNormal, optInFactorNarrow));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("SQZMOM", "inHigh", inHigh, guardInLen);
      requireLength("SQZMOM", "inLow", inLow, guardInLen);
      requireLength("SQZMOM", "inClose", inClose, guardInLen);
      requireLength("SQZMOM", "outMomentum", outMomentum, guardOutLen);
      requireLength("SQZMOM", "outSqueeze", outSqueeze, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = sqzmomImpl(startIdx, endIdx, inHigh, inLow, inClose, optInBBPeriod, optInNbDev, optInKCPeriod, optInFactorWide, optInFactorNormal, optInFactorNarrow, outBegIdx, outNBElement, outMomentum, outSqueeze);
      if( retCode != RetCode.SUCCESS ) {
         throw failure("SQZMOM", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live SQZMOM stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#sqzmom} over the same series.
    * Open with {@link Core#sqzmomOpen}; there is no close — the handle is
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
   public static final class SqzmomStream {
      private Core core;
      private int optInBBPeriod;
      private double optInNbDev;
      private int optInKCPeriod;
      private double optInFactorWide;
      private double optInFactorNormal;
      private double optInFactorNarrow;
      private double cur_outMomentum;
      private int cur_outSqueeze;
      private MidpriceStream sub0;
      private SmaStream sub1;
      private MedpriceStream sub2;
      private SmaStream sub3;
      private SubStream sub4;
      private TrangeStream sub5;
      private SmaStream sub6;
      private SmaStream sub7;
      private StddevStream sub8;
      private SmaStream sub9;
      private LinearregStream sub10;
      private int outRangeBegIdx;
      private int outRangeCount;

      private SqzmomStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#sqzmom} reports over the same bars: the
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
       * by one and nothing else moves — {@link #value(SqzmomOut)} keeps answering the previous
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
            throw failure("SQZMOM advance", RetCode.OUT_OF_RANGE_END_INDEX);
         this.outRangeCount++;
      }

      private SqzmomStream( SqzmomStream other ) {
         this.core = other.core;
         this.optInBBPeriod = other.optInBBPeriod;
         this.optInNbDev = other.optInNbDev;
         this.optInKCPeriod = other.optInKCPeriod;
         this.optInFactorWide = other.optInFactorWide;
         this.optInFactorNormal = other.optInFactorNormal;
         this.optInFactorNarrow = other.optInFactorNarrow;
         this.cur_outMomentum = other.cur_outMomentum;
         this.cur_outSqueeze = other.cur_outSqueeze;
         this.sub0 = new MidpriceStream(other.sub0);
         this.sub1 = new SmaStream(other.sub1);
         this.sub2 = new MedpriceStream(other.sub2);
         this.sub3 = new SmaStream(other.sub3);
         this.sub4 = new SubStream(other.sub4);
         this.sub5 = new TrangeStream(other.sub5);
         this.sub6 = new SmaStream(other.sub6);
         this.sub7 = new SmaStream(other.sub7);
         this.sub8 = new StddevStream(other.sub8);
         this.sub9 = new SmaStream(other.sub9);
         this.sub10 = new LinearregStream(other.sub10);
         this.outRangeBegIdx = other.outRangeBegIdx;
         this.outRangeCount = other.outRangeCount;
      }

      /**
       * Commit one closed bar, writing the new current values into the {@code out} the CALLER owns.
       * <p>Throws {@link IllegalArgumentException} if any bar value is not
       * finite (NaN or an infinity). That check runs before anything is
       * written, so nothing moves — {@link #outRange()} included — and
       * {@link #value(SqzmomOut)} still answers the previous value. Re-feed the bar when a
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
      public void update( double inHigh, double inLow, double inClose, SqzmomOut out ) {
         if( this.outRangeBegIdx + this.outRangeCount > INDEX_MAX )
            throw failure("SQZMOM update", RetCode.OUT_OF_RANGE_END_INDEX);
         requireArgument("SQZMOM update", "out", out);
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) || !Double.isFinite(inClose) )
            throw nonFiniteBar("SQZMOM update", !Double.isFinite(inHigh) ? "inHigh" : !Double.isFinite(inLow) ? "inLow" : "inClose");
         core.sqzmomStepImpl(this, inHigh, inLow, inClose);
         this.outRangeCount++;
         out.momentum = this.cur_outMomentum;
         out.squeeze = this.cur_outSqueeze;
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would write — the same
       * transition, with every store it would make carried in a local instead.
       * Never writes this handle, so peeks may run concurrently with each other.
       * <p>It counts no bar, so it keeps answering past the
       * {@link Core#INDEX_MAX} ceiling {@code update} stops at.
       */
      public void peek( double inHigh, double inLow, double inClose, SqzmomOut out ) {
         requireArgument("SQZMOM peek", "out", out);
         if( !Double.isFinite(inHigh) || !Double.isFinite(inLow) || !Double.isFinite(inClose) )
            throw nonFiniteBar("SQZMOM peek", !Double.isFinite(inHigh) ? "inHigh" : !Double.isFinite(inLow) ? "inLow" : "inClose");
         SqzmomStream sp = this;
         double band = 0.0;
         double centre = 0.0;
         double lo = 0.0;
         double up = 0.0;
         double cur_tempDev = 0.0;
         double cur_tempWork = 0.0;
         double cur_tempMid = 0.0;
         double cur_tempSD = 0.0;
         double cur_tempKCM = 0.0;
         double cur_outMomentum = 0.0;
         int cur_outSqueeze = 0;
         /* Pipeline the new bar through the sub-streams (batch tail order). */
         cur_tempDev = sp.sub0.peek(inHigh, inLow);
         cur_tempWork = sp.sub1.peek(inClose);
         cur_tempDev = sp.sub2.peek(cur_tempDev, cur_tempWork);
         cur_tempWork = sp.sub3.peek(inClose);
         cur_tempDev = sp.sub4.peek(cur_tempWork, cur_tempDev);
         cur_tempWork = sp.sub5.peek(inHigh, inLow, inClose);
         cur_tempWork = sp.sub6.peek(cur_tempWork);
         cur_tempMid = sp.sub7.peek(inClose);
         cur_tempSD = sp.sub8.peek(inClose);
         cur_tempKCM = sp.sub9.peek(inClose);
         cur_outMomentum = sp.sub10.peek(cur_tempDev);
         /* Combine map (batch tail, per bar). */
         up = Math.fma(cur_tempSD, sp.optInNbDev, cur_tempMid);
         lo = cur_tempMid - cur_tempSD * sp.optInNbDev;
         centre = cur_tempKCM;
         band = cur_tempWork;
         if( lo > centre - sp.optInFactorNarrow * band && up < centre + sp.optInFactorNarrow * band ) {
            cur_outSqueeze = 3;
         } else if( lo > centre - sp.optInFactorNormal * band && up < centre + sp.optInFactorNormal * band ) {
            cur_outSqueeze = 2;
         } else if( lo > centre - sp.optInFactorWide * band && up < centre + sp.optInFactorWide * band ) {
            cur_outSqueeze = 1;
         } else if( lo < centre - sp.optInFactorWide * band && up > centre + sp.optInFactorWide * band ) {
            cur_outSqueeze = -1;
         } else {
            cur_outSqueeze = 0;
         }
         out.momentum = cur_outMomentum;
         out.squeeze = cur_outSqueeze;
      }

      /**
       * The value at the last bar this stream counted — the bar
       * {@link #outRange()} ends on. The last history bar right after open,
       * then whatever the latest accepted {@code update} wrote.
       * A pure field read; {@code peek} does not change it. Overwrites {@code out}.
       */
      public void value( SqzmomOut out ) {
         requireArgument("SQZMOM value", "out", out);
         out.momentum = this.cur_outMomentum;
         out.squeeze = this.cur_outSqueeze;
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
      public SqzmomStream clone() {
         return new SqzmomStream(this);
      }
   }

   /**
    * The outputs of one SQZMOM bar, written by the stream into an object the
    * CALLER owns. Allocate one and reuse it: {@code update}, {@code peek}
    * and {@code value} overwrite its fields, so the sink itself costs
    * nothing per bar.
    *
    * <p><b>Its contents are only valid until the next call that writes it.</b>
    * It is a mutable buffer, not a reading: a reference kept past that call,
    * or one put in a collection, sees the value change underneath it. Copy the
    * fields out if the reading has to outlive the call.
    *
    * <p>Deliberately no {@code equals} or {@code hashCode}: a mutable type
    * with value equality breaks the {@code HashMap}/{@code HashSet}
    * invariant the moment a reused instance becomes a key. Compare the fields.
    */
   public static final class SqzmomOut {
      /** Linear regression of the close against the Donchian/SMA anchor. */
      public double momentum;
      /** Compression level: 3 narrow, 2 normal, 1 wide, -1 released, 0 otherwise. */
      public int squeeze;
   }
   private void sqzmomStepImpl( SqzmomStream sp, double inHigh, double inLow, double inClose )
   {
      double band = 0.0;
      double centre = 0.0;
      double lo = 0.0;
      double up = 0.0;
      double cur_tempDev = 0.0;
      double cur_tempWork = 0.0;
      double cur_tempMid = 0.0;
      double cur_tempSD = 0.0;
      double cur_tempKCM = 0.0;
      double cur_outMomentum = 0.0;
      int cur_outSqueeze = 0;
      /* Pipeline the new bar through the sub-streams (batch tail order). */
      cur_tempDev = sp.sub0.update(inHigh, inLow);
      cur_tempWork = sp.sub1.update(inClose);
      cur_tempDev = sp.sub2.update(cur_tempDev, cur_tempWork);
      cur_tempWork = sp.sub3.update(inClose);
      cur_tempDev = sp.sub4.update(cur_tempWork, cur_tempDev);
      cur_tempWork = sp.sub5.update(inHigh, inLow, inClose);
      cur_tempWork = sp.sub6.update(cur_tempWork);
      cur_tempMid = sp.sub7.update(inClose);
      cur_tempSD = sp.sub8.update(inClose);
      cur_tempKCM = sp.sub9.update(inClose);
      cur_outMomentum = sp.sub10.update(cur_tempDev);
      /* Combine map (batch tail, per bar). */
      up = Math.fma(cur_tempSD, sp.optInNbDev, cur_tempMid);
      lo = cur_tempMid - cur_tempSD * sp.optInNbDev;
      centre = cur_tempKCM;
      band = cur_tempWork;
      if( lo > centre - sp.optInFactorNarrow * band && up < centre + sp.optInFactorNarrow * band ) {
         cur_outSqueeze = 3;
      } else if( lo > centre - sp.optInFactorNormal * band && up < centre + sp.optInFactorNormal * band ) {
         cur_outSqueeze = 2;
      } else if( lo > centre - sp.optInFactorWide * band && up < centre + sp.optInFactorWide * band ) {
         cur_outSqueeze = 1;
      } else if( lo < centre - sp.optInFactorWide * band && up > centre + sp.optInFactorWide * band ) {
         cur_outSqueeze = -1;
      } else {
         cur_outSqueeze = 0;
      }
      sp.cur_outMomentum = cur_outMomentum;
      sp.cur_outSqueeze = cur_outSqueeze;
   }
   private RetCode sqzmomOpenImpl( SqzmomStream sp, double inHigh[], double inLow[], double inClose[], int startIdx, int optInBBPeriod, double optInNbDev, int optInKCPeriod, double optInFactorWide, double optInFactorNormal, double optInFactorNarrow, MInteger outBegIdx, MInteger outNBElement, double outMomentum[], int outSqueeze[], int outStride )
   {
      RetCode retCode;
      int lookbackTotal = 0;
      int stageLookback = 0;
      int stageIdx = 0;
      int stageLen = 0;
      int i = 0;
      int n = 0;
      MInteger tempBegIdx = new MInteger();
      MInteger tempNbElement = new MInteger();
      double[] tempDev;
      double[] tempWork;
      double[] tempKCM;
      double[] tempMid;
      double[] tempSD;
      double up = 0;
      double lo = 0;
      double centre = 0;
      double band = 0;
      int historyLen = inHigh.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.OUT_OF_RANGE_START_INDEX;
      }
      if( historyLen > INDEX_MAX + 1 ) {
         return RetCode.OUT_OF_RANGE_END_INDEX;
      }
      if( inLow.length != inHigh.length || inClose.length != inHigh.length ) {
         return RetCode.BAD_PARAM;
      }
      if( optInBBPeriod == Integer.MIN_VALUE ) {
         optInBBPeriod = 20;
      } else if( optInBBPeriod < 2 || optInBBPeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInNbDev == REAL_DEFAULT ) {
         optInNbDev = 2e0;
      } else if( !(optInNbDev >= 0e0 && optInNbDev <= REAL_MAX) ) {
         return RetCode.BAD_PARAM;
      }
      if( optInKCPeriod == Integer.MIN_VALUE ) {
         optInKCPeriod = 20;
      } else if( optInKCPeriod < 2 || optInKCPeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInFactorWide == REAL_DEFAULT ) {
         optInFactorWide = 2e0;
      } else if( !(optInFactorWide >= 0e0 && optInFactorWide <= REAL_MAX) ) {
         return RetCode.BAD_PARAM;
      }
      if( optInFactorNormal == REAL_DEFAULT ) {
         optInFactorNormal = 1.5e0;
      } else if( !(optInFactorNormal >= 0e0 && optInFactorNormal <= REAL_MAX) ) {
         return RetCode.BAD_PARAM;
      }
      if( optInFactorNarrow == REAL_DEFAULT ) {
         optInFactorNarrow = 1e0;
      } else if( !(optInFactorNarrow >= 0e0 && optInFactorNarrow <= REAL_MAX) ) {
         return RetCode.BAD_PARAM;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.INSUFFICIENT_HISTORY;
      }
      if( historyLen < sqzmomLookback(optInBBPeriod, optInNbDev, optInKCPeriod, optInFactorWide, optInFactorNormal, optInFactorNarrow) + 1 ) {
         return RetCode.INSUFFICIENT_HISTORY;
      }
      double[] sc_outMomentum = outStride == 1 ? outMomentum : new double[historyLen];
      int[] sc_outSqueeze = outStride == 1 ? outSqueeze : new int[historyLen];
      /* Every leg is a shipped function, CALLED rather than transcribed, so both
       * outputs carry each callee's own arithmetic bit for bit:
       *
       *   BBU, BBL = sma(close, b) +/- nbdev * stddev(close, b, 1.0)
       *   KCM      = sma(close, k)
       *   BAND     = sma(trange(high,low,close), k)      the simple mean of TR
       *   DMID     = midprice(high, low, k)
       *   DEV      = close - (DMID + KCM)/2
       *   momentum = linearreg(DEV, k)
       *
       * The bands are taken as the MA + STDDEV pair rather than through bbands
       * because bbands' SMA fast path states it is bit-identical to that pair, and
       * it is what bbands' own stream tier composes; taking the pair here keeps
       * every optional argument a parameter or a literal.
       *
       * DEV is built from shipped functions too, not in a loop of its own:
       * medprice is (a+b)/2 over any two series, and sma at a period of 1 is the
       * identity -- a one-element sum divided by 1.0, exact for every double --
       * which is what puts the close into the staged range's own indexing so that
       * sub can take the difference.
       *
       * Two chains need their input one window earlier than the first output bar:
       * sma(k) over TR, and linearreg(k) over DEV. Both windows are k-1 long, so
       * one staged range serves both.
       */
      lookbackTotal = sqzmomLookback(optInBBPeriod, optInNbDev, optInKCPeriod, optInFactorWide, optInFactorNormal, optInFactorNarrow);
      if( lookbackTotal > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.INSUFFICIENT_HISTORY ;
      }
      if( startIdx < lookbackTotal ) {
         startIdx = lookbackTotal;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.INSUFFICIENT_HISTORY ;
      }
      n = endIdx - startIdx + 1;
      stageLookback = linearregLookback(optInKCPeriod);
      stageIdx = startIdx - stageLookback;
      stageLen = n + stageLookback;
      tempDev = new double[(int)(stageLen * 1)];
      tempWork = new double[(int)(stageLen * 1)];
      tempKCM = new double[(int)(n * 1)];
      tempMid = new double[(int)(n * 1)];
      tempSD = new double[(int)(n * 1)];
      /* The momentum anchor: the Donchian midpoint and the Keltner centre, averaged.
       * midprice is bit-identical to donchian's middle band.
       */
      /* Sub-stream 0: midprice over `inHigh, inLow`, warmed from bar 0 up to the
       * sub-call's own startIdx (the seeding point). */
      MidpriceStream sub0 = midpriceOpenAndFillInternal(inHigh, inLow, stageIdx, optInKCPeriod, tempBegIdx, tempNbElement, tempDev);
      retCode = RetCode.SUCCESS;
      /* Sub-stream 1: sma over `inClose`, warmed from bar 0 up to the
       * sub-call's own startIdx (the seeding point). */
      SmaStream sub1 = smaOpenAndFillInternal(inClose, stageIdx, optInKCPeriod, tempBegIdx, tempNbElement, tempWork);
      retCode = RetCode.SUCCESS;
      /* tempDev becomes (DMID + KCM)/2 in place: medprice reads and writes the same
       * index, so the overwrite is safe.
       */
      /* Sub-stream 2: medprice over `tempDev, tempWork`, warmed from bar 0 up to the
       * sub-call's own startIdx (the seeding point). */
      MedpriceStream sub2 = medpriceOpenInternal(java.util.Arrays.copyOfRange(tempDev, 0, (stageLen - 1) + 1), java.util.Arrays.copyOfRange(tempWork, 0, (stageLen - 1) + 1), 0);
      OutRange _xr0 = medprice(0, stageLen - 1, tempDev, tempWork, tempDev);
      tempBegIdx.value = _xr0.begIdx();
      tempNbElement.value = _xr0.count();
      retCode = RetCode.SUCCESS;
      /* The close over the staged range, in that range's own indexing. */
      /* Sub-stream 3: sma over `inClose`, warmed from bar 0 up to the
       * sub-call's own startIdx (the seeding point). */
      SmaStream sub3 = smaOpenAndFillInternal(inClose, stageIdx, 1, tempBegIdx, tempNbElement, tempWork);
      retCode = RetCode.SUCCESS;
      /* Sub-stream 4: sub over `tempWork, tempDev`, warmed from bar 0 up to the
       * sub-call's own startIdx (the seeding point). */
      SubStream sub4 = subOpenInternal(java.util.Arrays.copyOfRange(tempWork, 0, (stageLen - 1) + 1), java.util.Arrays.copyOfRange(tempDev, 0, (stageLen - 1) + 1), 0);
      OutRange _xr1 = sub(0, stageLen - 1, tempWork, tempDev, tempDev);
      tempBegIdx.value = _xr1.begIdx();
      tempNbElement.value = _xr1.count();
      retCode = RetCode.SUCCESS;
      /* The compression band: the simple mean of the true range, not an ATR.
       * trange's own lookback is 1, and stageIdx is at least 1 because the
       * compression chain alone puts lookbackTotal at k or more.
       */
      /* Sub-stream 5: trange over `inHigh, inLow, inClose`, warmed from bar 0 up to the
       * sub-call's own startIdx (the seeding point). */
      TrangeStream sub5 = trangeOpenAndFillInternal(inHigh, inLow, inClose, stageIdx, tempBegIdx, tempNbElement, tempWork);
      retCode = RetCode.SUCCESS;
      /* Sub-stream 6: sma over `tempWork`, warmed from bar 0 up to the
       * sub-call's own startIdx (the seeding point). */
      SmaStream sub6 = smaOpenInternal(java.util.Arrays.copyOfRange(tempWork, 0, (stageLen - 1) + 1), 0, optInKCPeriod);
      OutRange _xr2 = sma(0, stageLen - 1, tempWork, optInKCPeriod, tempWork);
      tempBegIdx.value = _xr2.begIdx();
      tempNbElement.value = _xr2.count();
      retCode = RetCode.SUCCESS;
      /* Sub-stream 7: sma over `inClose`, warmed from bar 0 up to the
       * sub-call's own startIdx (the seeding point). */
      SmaStream sub7 = smaOpenAndFillInternal(inClose, startIdx, optInBBPeriod, tempBegIdx, tempNbElement, tempMid);
      retCode = RetCode.SUCCESS;
      /* Sub-stream 8: stddev over `inClose`, warmed from bar 0 up to the
       * sub-call's own startIdx (the seeding point). */
      StddevStream sub8 = stddevOpenAndFillInternal(inClose, startIdx, optInBBPeriod, 1.0, tempBegIdx, tempNbElement, tempSD);
      retCode = RetCode.SUCCESS;
      /* Sub-stream 9: sma over `inClose`, warmed from bar 0 up to the
       * sub-call's own startIdx (the seeding point). */
      SmaStream sub9 = smaOpenAndFillInternal(inClose, startIdx, optInKCPeriod, tempBegIdx, tempNbElement, tempKCM);
      retCode = RetCode.SUCCESS;
      /* LAST of the calls, because it is the first to write a caller buffer. Every
       * read of high, low and close is above it, so an output aliased onto one of
       * them -- which the abstract layer tests on purpose -- is still intact when it
       * is read.
       *
       * The regression window closes on the first output bar, so entering it at 0 of
       * the staged range puts its first fitted value on startIdx.
       */
      /* Sub-stream 10: linearreg over `tempDev`, warmed from bar 0 up to the
       * sub-call's own startIdx (the seeding point). */
      LinearregStream sub10 = linearregOpenAndFillInternal(java.util.Arrays.copyOfRange(tempDev, 0, (stageLen - 1) + 1), 0, optInKCPeriod, tempBegIdx, tempNbElement, sc_outMomentum);
      retCode = RetCode.SUCCESS;
      /* The ordinal. The three Keltner widths share the centre and BAND >= 0, so
       * the tests nest: inside(narrow) implies inside(normal) implies inside(wide).
       * Testing narrow first is what makes the ordinal monotone without comparing
       * the factors to each other, which is why the card asks for
       * `wide >= normal >= narrow >= 0` and not for a strict order.
       */
      for( i = 0; i < n; i += 1 ) {
         up = Math.fma(tempSD[i], optInNbDev, tempMid[i]);
         lo = tempMid[i] - tempSD[i] * optInNbDev;
         centre = tempKCM[i];
         band = tempWork[i];
         if( lo > centre - optInFactorNarrow * band && up < centre + optInFactorNarrow * band ) {
            sc_outSqueeze[i] = 3;
         } else if( lo > centre - optInFactorNormal * band && up < centre + optInFactorNormal * band ) {
            sc_outSqueeze[i] = 2;
         } else if( lo > centre - optInFactorWide * band && up < centre + optInFactorWide * band ) {
            sc_outSqueeze[i] = 1;
         } else if( lo < centre - optInFactorWide * band && up > centre + optInFactorWide * band ) {
            sc_outSqueeze[i] = -1;
         } else {
            sc_outSqueeze[i] = 0;
         }
      }
      outBegIdx.value = startIdx;
      outNBElement.value = n;
      /* Capture the live producer state + sub handles. */
      if( outNBElement.value < 1 ) {
         return RetCode.INSUFFICIENT_HISTORY;
      }
      sp.optInBBPeriod = optInBBPeriod;
      sp.optInNbDev = optInNbDev;
      sp.optInKCPeriod = optInKCPeriod;
      sp.optInFactorWide = optInFactorWide;
      sp.optInFactorNormal = optInFactorNormal;
      sp.optInFactorNarrow = optInFactorNarrow;
      sp.sub0 = sub0;
      sp.sub1 = sub1;
      sp.sub2 = sub2;
      sp.sub3 = sub3;
      sp.sub4 = sub4;
      sp.sub5 = sub5;
      sp.sub6 = sub6;
      sp.sub7 = sub7;
      sp.sub8 = sub8;
      sp.sub9 = sub9;
      sp.sub10 = sub10;
      sp.cur_outMomentum = sc_outMomentum[outNBElement.value - 1];
      sp.cur_outSqueeze = sc_outSqueeze[outNBElement.value - 1];
      return RetCode.SUCCESS;
   }
   /* sqzmomOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   SqzmomStream sqzmomOpenAndFillInternal( double inHigh[], double inLow[], double inClose[], int startIdx, int optInBBPeriod, double optInNbDev, int optInKCPeriod, double optInFactorWide, double optInFactorNormal, double optInFactorNarrow, MInteger outBegIdx, MInteger outNBElement, double outMomentum[], int outSqueeze[] )
   {
      SqzmomStream sp = new SqzmomStream(this);
      RetCode retCode = sqzmomOpenImpl(sp, inHigh, inLow, inClose, startIdx, optInBBPeriod, optInNbDev, optInKCPeriod, optInFactorWide, optInFactorNormal, optInFactorNarrow, outBegIdx, outNBElement, outMomentum, outSqueeze, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.SUCCESS ) {
         return sp;
      }
      if( retCode == RetCode.INSUFFICIENT_HISTORY ) {
         throw insufficientHistory("SQZMOM openAndFill", inHigh.length, startIdx, sqzmomLookback(optInBBPeriod, optInNbDev, optInKCPeriod, optInFactorWide, optInFactorNormal, optInFactorNarrow));
      }
      throw streamFailure("SQZMOM openAndFill", retCode);
   }
   /* Internal startIdx-anchored open behind sqzmomOpen (composition seam). */
   SqzmomStream sqzmomOpenInternal( double inHigh[], double inLow[], double inClose[], int startIdx, int optInBBPeriod, double optInNbDev, int optInKCPeriod, double optInFactorWide, double optInFactorNormal, double optInFactorNarrow )
   {
      SqzmomStream sp = new SqzmomStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outMomentum = new double[1];
      int[] sink_outSqueeze = new int[1];
      RetCode retCode = sqzmomOpenImpl(sp, inHigh, inLow, inClose, startIdx, optInBBPeriod, optInNbDev, optInKCPeriod, optInFactorWide, optInFactorNormal, optInFactorNarrow, outBegIdx, outNBElement, sink_outMomentum, sink_outSqueeze, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.SUCCESS ) {
         return sp;
      }
      if( retCode == RetCode.INSUFFICIENT_HISTORY ) {
         throw insufficientHistory("SQZMOM open", inHigh.length, startIdx, sqzmomLookback(optInBBPeriod, optInNbDev, optInKCPeriod, optInFactorWide, optInFactorNormal, optInFactorNarrow));
      }
      throw streamFailure("SQZMOM open", retCode);
   }
   /**
    * Open a live SQZMOM stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#sqzmom} at that bar.
    * <p>The history must hold at least {@code sqzmomLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@link Integer#MIN_VALUE} and {@link Core#REAL_DEFAULT} select a
    * parameter's documented default, as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public SqzmomStream sqzmomOpen( double inHigh[], double inLow[], double inClose[], int optInBBPeriod, double optInNbDev, int optInKCPeriod, double optInFactorWide, double optInFactorNormal, double optInFactorNarrow )
   {
      requireArgument("SQZMOM open", "inHigh", inHigh);
      requireHistory("SQZMOM open", inHigh.length);
      requireArgument("SQZMOM open", "inLow", inLow);
      requireArgument("SQZMOM open", "inClose", inClose);
      requireHistoryLength("SQZMOM open", "inLow", inLow.length, inHigh.length);
      requireHistoryLength("SQZMOM open", "inClose", inClose.length, inHigh.length);
      return sqzmomOpenInternal(inHigh, inLow, inClose, 0, optInBBPeriod, optInNbDev, optInKCPeriod, optInFactorWide, optInFactorNormal, optInFactorNarrow);
   }
   /**
    * {@link Core#sqzmomOpen} that also fills the output array(s) bit-identically
    * to {@link Core#sqzmom} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link SqzmomStream#outRange()}.
    */
   public SqzmomStream sqzmomOpenAndFill( double inHigh[], double inLow[], double inClose[], int optInBBPeriod, double optInNbDev, int optInKCPeriod, double optInFactorWide, double optInFactorNormal, double optInFactorNarrow, double outMomentum[], int outSqueeze[] )
   {
      requireArgument("SQZMOM openAndFill", "inHigh", inHigh);
      requireHistory("SQZMOM openAndFill", inHigh.length);
      requireArgument("SQZMOM openAndFill", "inLow", inLow);
      requireArgument("SQZMOM openAndFill", "inClose", inClose);
      int guardOutLen = openFillCount("SQZMOM openAndFill", inHigh.length, sqzmomLookback(optInBBPeriod, optInNbDev, optInKCPeriod, optInFactorWide, optInFactorNormal, optInFactorNarrow));
      requireHistoryLength("SQZMOM openAndFill", "inLow", inLow.length, inHigh.length);
      requireHistoryLength("SQZMOM openAndFill", "inClose", inClose.length, inHigh.length);
      requireLength("SQZMOM openAndFill", "outMomentum", outMomentum, guardOutLen);
      requireLength("SQZMOM openAndFill", "outSqueeze", outSqueeze, guardOutLen);
      if( (Object)outMomentum == (Object)inHigh || (Object)outMomentum == (Object)inLow || (Object)outMomentum == (Object)inClose || (Object)outSqueeze == (Object)inHigh || (Object)outSqueeze == (Object)inLow || (Object)outSqueeze == (Object)inClose || (Object)outMomentum == (Object)outSqueeze ) {
         throw streamFailure("SQZMOM openAndFill", RetCode.BAD_PARAM);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return sqzmomOpenAndFillInternal(inHigh, inLow, inClose, 0, optInBBPeriod, optInNbDev, optInKCPeriod, optInFactorWide, optInFactorNormal, optInFactorNarrow, outBegIdx, outNBElement, outMomentum, outSqueeze);
   }
