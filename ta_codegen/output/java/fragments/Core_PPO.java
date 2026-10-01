/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  AA       Andrew Atkinson
 *  CC       Claude Code (AI assistant)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  112400 MF     Template creation.
 *  052603 MF     Adapt code to compile with .NET Managed C++
 *  020605 AA     Fix #1117666 Lookback bug.
 *  071126 MF,CC  Rewrite the combine into flat error-guards and a single-cursor
 *                offset index (offset = fastNb - *outNBElement). Bit-identical,
 *                streamable, and index-safe; the TA_IS_ZERO guard is unchanged.
 *  092726 MF,CC  0 on a slow window of zero bars for the windowed MA types (#454).
 *  092826 MF,CC  #459 fuse the fast and slow SMA into one pass over the input:
 *                two running sums, no intermediate buffer, no allocation.
 *                Bit-identical.
 *  092826 MF,CC  Fuse the fast and slow EMA into one pass (#459).
 */

   /**
    * Number of leading input bars {@link Core#ppo} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInFastPeriod Period of the fast MA (default 12; range 2..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @param optInSlowPeriod Period of the slow MA (default 26; range 2..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @param optInMAType Moving average type used for both MAs (default 1 = EMA;
    *        values: 0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA,
    *        8=T3, 9=HMA, 10=DISABLED, 11=DEFAULT, 12=ZLEMA, 13=RMA, 14=VIDYA, 15=ALMA;
    *        {@code MAType.DEFAULT} selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int ppoLookback( int optInFastPeriod, int optInSlowPeriod, MAType optInMAType )
   {
      if( optInMAType == null ) {
         return -1;
      }
      if( optInFastPeriod == Integer.MIN_VALUE ) {
         optInFastPeriod = 12;
      } else if( optInFastPeriod < 2 || optInFastPeriod > 100000 ) {
         return -1;
      }
      if( optInSlowPeriod == Integer.MIN_VALUE ) {
         optInSlowPeriod = 26;
      } else if( optInSlowPeriod < 2 || optInSlowPeriod > 100000 ) {
         return -1;
      }
      if( optInMAType == MAType.DEFAULT ) {
         optInMAType = MAType.EMA;
      }
      /* Lookback is driven by the slowest MA. */
      return maLookback(Math.max(optInSlowPeriod, optInFastPeriod), optInMAType) ;

   }
   /**
    * How many bars ahead (positive) or behind (negative) of the bar that
    * computed it a chart draws one output of {@link Core#ppo}.
    * <p>Every output of this function is drawn at its own bar, so the answer is
    * 0.
    *
    * @param optInFastPeriod Period of the fast MA (default 12; range 2..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @param optInSlowPeriod Period of the slow MA (default 26; range 2..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @param optInMAType Moving average type used for both MAs (default 1 = EMA;
    *        values: 0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA,
    *        8=T3, 9=HMA, 10=DISABLED, 11=DEFAULT, 12=ZLEMA, 13=RMA, 14=VIDYA, 15=ALMA;
    *        {@code MAType.DEFAULT} selects the default).
    * @param outputIdx Position of the output in the batch signature, from 0.
    * @return The display shift, or {@code Integer.MIN_VALUE} if a parameter is
    *        out of range or the index names no output.
    */
   public int ppoDisplayShift( int optInFastPeriod, int optInSlowPeriod, MAType optInMAType, int outputIdx )
   {
      if( ppoLookback( optInFastPeriod, optInSlowPeriod, optInMAType ) < 0 ) {
         return Integer.MIN_VALUE;
      }
      if( outputIdx < 0 || outputIdx >= 1 ) {
         return Integer.MIN_VALUE;
      }
      return 0;
   }
   RetCode ppoImpl( int startIdx,
                    int endIdx,
                    double inReal[],
                    int optInFastPeriod,
                    int optInSlowPeriod,
                    MAType optInMAType,
                    MInteger outBegIdx,
                    MInteger outNBElement,
                    double outReal[] )
   {
      double[] tempBuffer;
      RetCode retCode;
      double tempReal = 0;
      int tempInteger = 0;
      MInteger fastBeg = new MInteger();
      MInteger fastNb = new MInteger();
      int offset = 0;
      int slowLookback = 0;
      int windowed = 0;
      int zeroRun = 0;
      int i = 0;
      if( (startIdx < 0) || (startIdx > INDEX_MAX) ) {
         return RetCode.OUT_OF_RANGE_START_INDEX ;
      }
      if( (endIdx < 0) || (endIdx > INDEX_MAX) || (endIdx < startIdx)) {
         return RetCode.OUT_OF_RANGE_END_INDEX ;
      }
      if( optInFastPeriod == Integer.MIN_VALUE ) {
         optInFastPeriod = 12;
      } else if( optInFastPeriod < 2 || optInFastPeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInSlowPeriod == Integer.MIN_VALUE ) {
         optInSlowPeriod = 26;
      } else if( optInSlowPeriod < 2 || optInSlowPeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInMAType == MAType.DEFAULT ) {
         optInMAType = MAType.EMA;
      }
      /* Nothing to produce: the range ends before the lookback. Return before
       * touching anything.
       *
       * Without this the fast MA below runs first, and its lookback is SMALLER
       * than ppo's own — so it reads the whole range and computes a result the
       * empty slow MA then discards. Observably identical (the slow MA's own early
       * return already yields 0,0 here), but it is the difference between "a range
       * that ends before the lookback reads nothing" being true of this function and
       * being false: with a caller-supplied inReal that stops short of endIdx, that
       * discarded work is an out-of-bounds read. Pinned by the zero-length no-I/O
       * probe over every guarded core.
       */
      if( maLookback(Math.max(optInSlowPeriod, optInFastPeriod), optInMAType) > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.SUCCESS ;
      }
      if( optInMAType == MAType.SMA ) {
         /* SMA fast path: the fast window is the newest optInFastPeriod bars of the
          * slow one, so ONE pass over the input serves both moving averages - two
          * running sums, no intermediate buffer and no allocation, where the general
          * path below makes two passes and allocates the fast MA in full.
          *
          * Bit-identical to that path. Each sum sees exactly the add/subtract
          * sequence TA_SMA gives it at its own period, starting from its own first
          * output bar - which is why the fast sum is walked alone over the bars the
          * slow MA does not reach (its running total is path-dependent, so arriving
          * at the first output bar by a shorter route would change the low bits) -
          * and each quotient is formed as sma.c forms it: the total AFTER adding the
          * new bar and BEFORE dropping the trailing one, divided by the period.
          *
          * SMA is one of the windowed types, so the dead-window rule of #454 applies
          * here too: _zeroRun is the same counter the general path keeps, warmed over
          * the same bars (which are the bars the slow sum is seeded from) and held at
          * the slow lookback once the window is dead.
          *
          * inReal may alias outReal, as it may in the general path. outReal[_outIdx]
          * is written at bar _i with _outIdx <= _i-optInSlowPeriod+1 <= both trailing
          * indices, and every read of bar _i happens before that write, so no bar is
          * overwritten before its last read.
          *
          * Every read is inside [0, endIdx]: the guard above leaves the slow
          * lookback no greater than endIdx, and the public tier rejects
          * endIdx < startIdx, so _slowStart <= endIdx and the seeding loops stop
          * one bar below it. There is nothing left for an empty-output arm to
          * catch, which is why this path has none.
          */
         double _fastTotal;
         double _slowTotal;
         double _fastValue;
         double _slowValue;
         double _slowMA;
         int _i;
         int _j;
         int _outIdx;
         int _fastStart;
         int _slowStart;
         int _fastTrailing;
         int _slowTrailing;
         int _slowLookback;
         int _zeroRun;
         /* Make sure slow is really slower than the fast period! if not, swap... */
         if( optInSlowPeriod < optInFastPeriod ) {
            tempInteger = optInSlowPeriod;
            optInSlowPeriod = optInFastPeriod;
            optInFastPeriod = tempInteger;
         }
         _fastStart = optInFastPeriod - 1;
         if( _fastStart < startIdx ) {
            _fastStart = startIdx;
         }
         _slowStart = optInSlowPeriod - 1;
         if( _slowStart < startIdx ) {
            _slowStart = startIdx;
         }
         _fastTrailing = _fastStart - (optInFastPeriod - 1);
         _fastTotal = 0.0;
         for( _j = _fastTrailing; _j < _fastStart; _j += 1 ) {
            _fastTotal += inReal[_j];
         }
         /* One loop seeds the slow sum and warms the dead-window counter: the bars
          * it walks, [_slowStart-_slowLookback, _slowStart), are exactly the ones
          * the general path warms _zeroRun over.
          */
         _slowLookback = optInSlowPeriod - 1;
         _zeroRun = 0;
         _slowTrailing = _slowStart - _slowLookback;
         _slowTotal = 0.0;
         for( _j = _slowTrailing; _j < _slowStart; _j += 1 ) {
            _slowTotal += inReal[_j];
            _zeroRun = (Math.abs(inReal[_j]) <= 0.0) ? _zeroRun + 1 : 0;
         }
         /* The bars the fast MA has and the slow one does not: advance the fast sum
          * alone. No output, but the sum must arrive at _slowStart along the same
          * path TA_SMA would have taken.
          */
         for( _i = _fastStart; _i < _slowStart; _i += 1 ) {
            _fastTotal += inReal[_i];
            _fastTotal -= inReal[_fastTrailing];
            _fastTrailing += 1;
         }
         _outIdx = 0;
         for( _i = _slowStart; _i <= endIdx; _i += 1 ) {
            _zeroRun = (Math.abs(inReal[_i]) <= 0.0) ? _zeroRun + 1 : 0;
            _fastTotal += inReal[_i];
            _fastValue = _fastTotal;
            _fastTotal -= inReal[_fastTrailing];
            _fastTrailing += 1;
            _slowTotal += inReal[_i];
            _slowValue = _slowTotal;
            _slowTotal -= inReal[_slowTrailing];
            _slowTrailing += 1;
            _slowMA = _slowValue / (double)optInSlowPeriod;
            if( _zeroRun > _slowLookback ) {
               _zeroRun = _slowLookback;
               outReal[_outIdx] = 0.0;
            } else if( !((-0.00000000000001 < _slowMA) && (_slowMA < 0.00000000000001)) ) {
               outReal[_outIdx] = (_fastValue / (double)optInFastPeriod - _slowMA) / _slowMA * 100.0;
            } else {
               outReal[_outIdx] = 0.0;
            }
            _outIdx += 1;
         }
         outBegIdx.value = _slowStart;
         outNBElement.value = _outIdx;
         return RetCode.SUCCESS ;
      }
      if( optInMAType == MAType.EMA ) {
         /* EMA fast path: both recursions in one loop, no buffer. Bit-identical to
          * the general path only while each EMA is seeded at its OWN lookback and
          * keeps ema.c's recursion spelling: the fast EMA starts earlier than the
          * slow one, and a shared seed bar would change every output.
          */
         double _eFastK;
         double _eSlowK;
         double _eFast;
         double _eSlow;
         double _eX;
         int _eN;
         int _eToday;
         int _eFastToday;
         int _eSlowToday;
         int _eSlowStart;
         int _eOutIdx;
         if( optInSlowPeriod < optInFastPeriod ) {
            tempInteger = optInSlowPeriod;
            optInSlowPeriod = optInFastPeriod;
            optInFastPeriod = tempInteger;
         }
         _eFastK = 2.0 / (double)(optInFastPeriod + 1);
         _eSlowK = 2.0 / (double)(optInSlowPeriod + 1);
         _eFastToday = emaLookback(optInFastPeriod);
         if( _eFastToday < startIdx ) {
            _eFastToday = startIdx;
         }
         _eFastToday -= emaLookback(optInFastPeriod);
         _eSlowStart = emaLookback(optInSlowPeriod);
         if( _eSlowStart < startIdx ) {
            _eSlowStart = startIdx;
         }
         _eSlowToday = _eSlowStart - emaLookback(optInSlowPeriod);
         _eFast = 0.0;
         for( _eN = 0; _eN < optInFastPeriod; _eN += 1 ) {
            _eFast += inReal[_eFastToday++];
         }
         _eFast = _eFast / optInFastPeriod;
         while( _eFastToday <= _eSlowStart ) {
            _eFast = Math.fma(inReal[_eFastToday++] - _eFast, _eFastK, _eFast);
         }
         _eSlow = 0.0;
         for( _eN = 0; _eN < optInSlowPeriod; _eN += 1 ) {
            _eSlow += inReal[_eSlowToday++];
         }
         _eSlow = _eSlow / optInSlowPeriod;
         while( _eSlowToday <= _eSlowStart ) {
            _eSlow = Math.fma(inReal[_eSlowToday++] - _eSlow, _eSlowK, _eSlow);
         }
         _eOutIdx = 0;
         if( !((-0.00000000000001 < _eSlow) && (_eSlow < 0.00000000000001)) ) {
            outReal[_eOutIdx] = (_eFast - _eSlow) / _eSlow * 100.0;
         } else {
            outReal[_eOutIdx] = 0.0;
         }
         _eOutIdx += 1;
         _eToday = _eSlowStart + 1;
         while( _eToday <= endIdx ) {
            _eX = inReal[_eToday++];
            _eFast = Math.fma(_eX - _eFast, _eFastK, _eFast);
            _eSlow = Math.fma(_eX - _eSlow, _eSlowK, _eSlow);
            if( !((-0.00000000000001 < _eSlow) && (_eSlow < 0.00000000000001)) ) {
               outReal[_eOutIdx] = (_eFast - _eSlow) / _eSlow * 100.0;
            } else {
               outReal[_eOutIdx] = 0.0;
            }
            _eOutIdx += 1;
         }
         outBegIdx.value = _eSlowStart;
         outNBElement.value = _eOutIdx;
         return RetCode.SUCCESS ;
      }
      /* Allocate an intermediate buffer. */
      tempBuffer = new double[(int)((endIdx - startIdx + 1) * 1)];
      /* Make sure slow is really slower than
       * the fast period! if not, swap...
       */
      if( optInSlowPeriod < optInFastPeriod ) {
         /* swap */
         tempInteger = optInSlowPeriod;
         optInSlowPeriod = optInFastPeriod;
         optInFastPeriod = tempInteger;
      }
      /* Calculate the fast MA into the tempBuffer. */
      OutRange _xr0 = ma(startIdx, endIdx, inReal, optInFastPeriod, optInMAType, tempBuffer);
      fastBeg.value = _xr0.begIdx();
      fastNb.value = _xr0.count();
      retCode = RetCode.SUCCESS;
      /* Calculate the slow MA into the output. */
      OutRange _xr1 = ma(startIdx, endIdx, inReal, optInSlowPeriod, optInMAType, outReal);
      outBegIdx.value = _xr1.begIdx();
      outNBElement.value = _xr1.count();
      retCode = RetCode.SUCCESS;
      /* fastNb - *outNBElement == slowBeg - fastBeg (the fast MA has at least as
       * many outputs), so tempBuffer[i+offset] is the fast MA at the same bar as
       * outReal[i], with a non-negative index. An empty slow MA skips the loop.
       */
      offset = fastNb.value - outNBElement.value;
      /* A windowed slow MA (SMA, WMA, TRIMA, HMA) over bars that are all exactly
       * zero is exactly zero, but its running sums leave residue there that
       * TA_IS_ZERO does not catch, and residue over residue is noise where 0 is
       * documented. zeroRun counts the trailing zero bars, held at slowLookback once
       * the window is dead. The recursive MA types really are nonzero on such a
       * window, so they keep the plain loop.
       */
      slowLookback = maLookback(optInSlowPeriod, optInMAType);
      windowed = (optInMAType == MAType.SMA || optInMAType == MAType.WMA || optInMAType == MAType.TRIMA || optInMAType == MAType.HMA) ? 1 : 0;
      zeroRun = 0;
      for( i = outBegIdx.value - slowLookback; i < outBegIdx.value; i += 1 ) {
         zeroRun = (Math.abs(inReal[i]) <= 0.0) ? zeroRun + 1 : 0;
      }
      if( windowed != 0 ) {
         for( i = 0; i < (int)outNBElement.value; i += 1 ) {
            zeroRun = (Math.abs(inReal[outBegIdx.value + i]) <= 0.0) ? zeroRun + 1 : 0;
            tempReal = outReal[i];
            if( zeroRun > slowLookback ) {
               zeroRun = slowLookback;
               outReal[i] = 0.0;
            } else if( !((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001)) ) {
               outReal[i] = (tempBuffer[i + offset] - tempReal) / tempReal * 100.0;
            } else {
               outReal[i] = 0.0;
            }
         }
      } else {
         /* Calculate ((fast MA)-(slow MA))/(slow MA) in the output. */
         for( i = 0; i < (int)outNBElement.value; i += 1 ) {
            tempReal = outReal[i];
            if( !((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001)) ) {
               outReal[i] = (tempBuffer[i + offset] - tempReal) / tempReal * 100.0;
            } else {
               outReal[i] = 0.0;
            }
         }
      }
      return RetCode.SUCCESS ;
   }
   RetCode ppoImpl( int startIdx,
                    int endIdx,
                    float inReal[],
                    int optInFastPeriod,
                    int optInSlowPeriod,
                    MAType optInMAType,
                    MInteger outBegIdx,
                    MInteger outNBElement,
                    double outReal[] )
   {
      double[] tempBuffer;
      RetCode retCode;
      double tempReal = 0;
      int tempInteger = 0;
      MInteger fastBeg = new MInteger();
      MInteger fastNb = new MInteger();
      int offset = 0;
      int slowLookback = 0;
      int windowed = 0;
      int zeroRun = 0;
      int i = 0;
      if( (startIdx < 0) || (startIdx > INDEX_MAX) ) {
         return RetCode.OUT_OF_RANGE_START_INDEX ;
      }
      if( (endIdx < 0) || (endIdx > INDEX_MAX) || (endIdx < startIdx)) {
         return RetCode.OUT_OF_RANGE_END_INDEX ;
      }
      if( optInFastPeriod == Integer.MIN_VALUE ) {
         optInFastPeriod = 12;
      } else if( optInFastPeriod < 2 || optInFastPeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInSlowPeriod == Integer.MIN_VALUE ) {
         optInSlowPeriod = 26;
      } else if( optInSlowPeriod < 2 || optInSlowPeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInMAType == MAType.DEFAULT ) {
         optInMAType = MAType.EMA;
      }
      if( maLookback(Math.max(optInSlowPeriod, optInFastPeriod), optInMAType) > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.SUCCESS ;
      }
      if( optInMAType == MAType.SMA ) {
         double _fastTotal;
         double _slowTotal;
         double _fastValue;
         double _slowValue;
         double _slowMA;
         int _i;
         int _j;
         int _outIdx;
         int _fastStart;
         int _slowStart;
         int _fastTrailing;
         int _slowTrailing;
         int _slowLookback;
         int _zeroRun;
         if( optInSlowPeriod < optInFastPeriod ) {
            tempInteger = optInSlowPeriod;
            optInSlowPeriod = optInFastPeriod;
            optInFastPeriod = tempInteger;
         }
         _fastStart = optInFastPeriod - 1;
         if( _fastStart < startIdx ) {
            _fastStart = startIdx;
         }
         _slowStart = optInSlowPeriod - 1;
         if( _slowStart < startIdx ) {
            _slowStart = startIdx;
         }
         _fastTrailing = _fastStart - (optInFastPeriod - 1);
         _fastTotal = 0.0;
         for( _j = _fastTrailing; _j < _fastStart; _j += 1 ) {
            _fastTotal += (double)inReal[_j];
         }
         _slowLookback = optInSlowPeriod - 1;
         _zeroRun = 0;
         _slowTrailing = _slowStart - _slowLookback;
         _slowTotal = 0.0;
         for( _j = _slowTrailing; _j < _slowStart; _j += 1 ) {
            _slowTotal += (double)inReal[_j];
            _zeroRun = (Math.abs((double)inReal[_j]) <= 0.0) ? _zeroRun + 1 : 0;
         }
         for( _i = _fastStart; _i < _slowStart; _i += 1 ) {
            _fastTotal += (double)inReal[_i];
            _fastTotal -= (double)inReal[_fastTrailing];
            _fastTrailing += 1;
         }
         _outIdx = 0;
         for( _i = _slowStart; _i <= endIdx; _i += 1 ) {
            _zeroRun = (Math.abs((double)inReal[_i]) <= 0.0) ? _zeroRun + 1 : 0;
            _fastTotal += (double)inReal[_i];
            _fastValue = _fastTotal;
            _fastTotal -= (double)inReal[_fastTrailing];
            _fastTrailing += 1;
            _slowTotal += (double)inReal[_i];
            _slowValue = _slowTotal;
            _slowTotal -= (double)inReal[_slowTrailing];
            _slowTrailing += 1;
            _slowMA = _slowValue / (double)optInSlowPeriod;
            if( _zeroRun > _slowLookback ) {
               _zeroRun = _slowLookback;
               outReal[_outIdx] = 0.0;
            } else if( !((-0.00000000000001 < _slowMA) && (_slowMA < 0.00000000000001)) ) {
               outReal[_outIdx] = (_fastValue / (double)optInFastPeriod - _slowMA) / _slowMA * 100.0;
            } else {
               outReal[_outIdx] = 0.0;
            }
            _outIdx += 1;
         }
         outBegIdx.value = _slowStart;
         outNBElement.value = _outIdx;
         return RetCode.SUCCESS ;
      }
      if( optInMAType == MAType.EMA ) {
         double _eFastK;
         double _eSlowK;
         double _eFast;
         double _eSlow;
         double _eX;
         int _eN;
         int _eToday;
         int _eFastToday;
         int _eSlowToday;
         int _eSlowStart;
         int _eOutIdx;
         if( optInSlowPeriod < optInFastPeriod ) {
            tempInteger = optInSlowPeriod;
            optInSlowPeriod = optInFastPeriod;
            optInFastPeriod = tempInteger;
         }
         _eFastK = 2.0 / (double)(optInFastPeriod + 1);
         _eSlowK = 2.0 / (double)(optInSlowPeriod + 1);
         _eFastToday = emaLookback(optInFastPeriod);
         if( _eFastToday < startIdx ) {
            _eFastToday = startIdx;
         }
         _eFastToday -= emaLookback(optInFastPeriod);
         _eSlowStart = emaLookback(optInSlowPeriod);
         if( _eSlowStart < startIdx ) {
            _eSlowStart = startIdx;
         }
         _eSlowToday = _eSlowStart - emaLookback(optInSlowPeriod);
         _eFast = 0.0;
         for( _eN = 0; _eN < optInFastPeriod; _eN += 1 ) {
            _eFast += (double)inReal[_eFastToday++];
         }
         _eFast = _eFast / optInFastPeriod;
         while( _eFastToday <= _eSlowStart ) {
            _eFast = Math.fma((double)inReal[_eFastToday++] - _eFast, _eFastK, _eFast);
         }
         _eSlow = 0.0;
         for( _eN = 0; _eN < optInSlowPeriod; _eN += 1 ) {
            _eSlow += (double)inReal[_eSlowToday++];
         }
         _eSlow = _eSlow / optInSlowPeriod;
         while( _eSlowToday <= _eSlowStart ) {
            _eSlow = Math.fma((double)inReal[_eSlowToday++] - _eSlow, _eSlowK, _eSlow);
         }
         _eOutIdx = 0;
         if( !((-0.00000000000001 < _eSlow) && (_eSlow < 0.00000000000001)) ) {
            outReal[_eOutIdx] = (_eFast - _eSlow) / _eSlow * 100.0;
         } else {
            outReal[_eOutIdx] = 0.0;
         }
         _eOutIdx += 1;
         _eToday = _eSlowStart + 1;
         while( _eToday <= endIdx ) {
            _eX = (double)inReal[_eToday++];
            _eFast = Math.fma(_eX - _eFast, _eFastK, _eFast);
            _eSlow = Math.fma(_eX - _eSlow, _eSlowK, _eSlow);
            if( !((-0.00000000000001 < _eSlow) && (_eSlow < 0.00000000000001)) ) {
               outReal[_eOutIdx] = (_eFast - _eSlow) / _eSlow * 100.0;
            } else {
               outReal[_eOutIdx] = 0.0;
            }
            _eOutIdx += 1;
         }
         outBegIdx.value = _eSlowStart;
         outNBElement.value = _eOutIdx;
         return RetCode.SUCCESS ;
      }
      tempBuffer = new double[(int)((endIdx - startIdx + 1) * 1)];
      if( optInSlowPeriod < optInFastPeriod ) {
         tempInteger = optInSlowPeriod;
         optInSlowPeriod = optInFastPeriod;
         optInFastPeriod = tempInteger;
      }
      OutRange _xr0 = ma(startIdx, endIdx, inReal, optInFastPeriod, optInMAType, tempBuffer);
      fastBeg.value = _xr0.begIdx();
      fastNb.value = _xr0.count();
      retCode = RetCode.SUCCESS;
      OutRange _xr1 = ma(startIdx, endIdx, inReal, optInSlowPeriod, optInMAType, outReal);
      outBegIdx.value = _xr1.begIdx();
      outNBElement.value = _xr1.count();
      retCode = RetCode.SUCCESS;
      offset = fastNb.value - outNBElement.value;
      slowLookback = maLookback(optInSlowPeriod, optInMAType);
      windowed = (optInMAType == MAType.SMA || optInMAType == MAType.WMA || optInMAType == MAType.TRIMA || optInMAType == MAType.HMA) ? 1 : 0;
      zeroRun = 0;
      for( i = outBegIdx.value - slowLookback; i < outBegIdx.value; i += 1 ) {
         zeroRun = (Math.abs((double)inReal[i]) <= 0.0) ? zeroRun + 1 : 0;
      }
      if( windowed != 0 ) {
         for( i = 0; i < (int)outNBElement.value; i += 1 ) {
            zeroRun = (Math.abs((double)inReal[outBegIdx.value + i]) <= 0.0) ? zeroRun + 1 : 0;
            tempReal = outReal[i];
            if( zeroRun > slowLookback ) {
               zeroRun = slowLookback;
               outReal[i] = 0.0;
            } else if( !((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001)) ) {
               outReal[i] = (tempBuffer[i + offset] - tempReal) / tempReal * 100.0;
            } else {
               outReal[i] = 0.0;
            }
         }
      } else {
         for( i = 0; i < (int)outNBElement.value; i += 1 ) {
            tempReal = outReal[i];
            if( !((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001)) ) {
               outReal[i] = (tempBuffer[i + offset] - tempReal) / tempReal * 100.0;
            } else {
               outReal[i] = 0.0;
            }
         }
      }
      return RetCode.SUCCESS ;
   }
   /**
    * Percentage Price Oscillator: the difference between a fast and slow moving
    * average expressed as a percentage of the slow MA. A normalized
    * (scale-invariant) variant of APO. Positive when the fast MA is above the
    * slow MA (upward momentum), negative otherwise; magnitude is the %
    * deviation.
    * <p>Formula and more info at <a
    * href="https://ta-lib.org/functions/ppo">ta-lib.org/functions/ppo</a>.
    * <p><b>Notes</b>
    * <ul>
    * <li>{@code optInMAType} applies to both the fast and slow moving average. {@code TA_MAType_MAMA} ignores its period argument, so with {@code optInMAType = TA_MAType_MAMA} the fast and slow MAs are identical, making the numerator — and therefore the output — zero at every bar.</li>
    * <li>If the slow period is set smaller than the fast period, the two are swapped, as in {@code MACD}.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are, and the
    * library never pads with NaN. A valid range that ends before
    * {@link Core#ppoLookback} is a <b>success with no values</b>
    * ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Input data series.
    * @param optInFastPeriod Period of the fast MA (default 12; range 2..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @param optInSlowPeriod Period of the slow MA (default 26; range 2..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @param optInMAType Moving average type used for both MAs (default 1 = EMA;
    *        values: 0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA,
    *        8=T3, 9=HMA, 10=DISABLED, 11=DEFAULT, 12=ZLEMA, 13=RMA, 14=VIDYA, 15=ALMA;
    *        {@code MAType.DEFAULT} selects the default).
    * @param outReal PPO value in percent. Must hold at least
    *        {@code endIdx - max(startIdx, ppoLookback(...)) + 1} values, the count the
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
    * @see Core#apo
    * @see Core#macd
    * @see Core#ma
    */
   public OutRange ppo( int startIdx,
                        int endIdx,
                        double inReal[],
                        int optInFastPeriod,
                        int optInSlowPeriod,
                        MAType optInMAType,
                        double outReal[] )
   {
      requireIndexRange("PPO", startIdx, endIdx);
      requireArgument("PPO", "optInMAType", optInMAType);
      int guardStart = clampedStart("PPO", startIdx, ppoLookback(optInFastPeriod, optInSlowPeriod, optInMAType));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("PPO", "inReal", inReal, guardInLen);
      requireLength("PPO", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = ppoImpl(startIdx, endIdx, inReal, optInFastPeriod, optInSlowPeriod, optInMAType, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.SUCCESS ) {
         throw failure("PPO", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Percentage Price Oscillator: the difference between a fast and slow moving
    * average expressed as a percentage of the slow MA. A normalized
    * (scale-invariant) variant of APO. Positive when the fast MA is above the
    * slow MA (upward momentum), negative otherwise; magnitude is the %
    * deviation.
    * <p>Formula and more info at <a
    * href="https://ta-lib.org/functions/ppo">ta-lib.org/functions/ppo</a>.
    * <p><b>Notes</b>
    * <ul>
    * <li>{@code optInMAType} applies to both the fast and slow moving average. {@code TA_MAType_MAMA} ignores its period argument, so with {@code optInMAType = TA_MAType_MAMA} the fast and slow MAs are identical, making the numerator — and therefore the output — zero at every bar.</li>
    * <li>If the slow period is set smaller than the fast period, the two are swapped, as in {@code MACD}.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are, and the
    * library never pads with NaN. A valid range that ends before
    * {@link Core#ppoLookback} is a <b>success with no values</b>
    * ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Input data series.
    * @param optInFastPeriod Period of the fast MA (default 12; range 2..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @param optInSlowPeriod Period of the slow MA (default 26; range 2..100000;
    *        {@code Integer.MIN_VALUE} selects the default).
    * @param optInMAType Moving average type used for both MAs (default 1 = EMA;
    *        values: 0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA,
    *        8=T3, 9=HMA, 10=DISABLED, 11=DEFAULT, 12=ZLEMA, 13=RMA, 14=VIDYA, 15=ALMA;
    *        {@code MAType.DEFAULT} selects the default).
    * @param outReal PPO value in percent. Must hold at least
    *        {@code endIdx - max(startIdx, ppoLookback(...)) + 1} values, the count the
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
    * @see Core#apo
    * @see Core#macd
    * @see Core#ma
    */
   public OutRange ppo( int startIdx,
                        int endIdx,
                        float inReal[],
                        int optInFastPeriod,
                        int optInSlowPeriod,
                        MAType optInMAType,
                        double outReal[] )
   {
      requireIndexRange("PPO", startIdx, endIdx);
      requireArgument("PPO", "optInMAType", optInMAType);
      int guardStart = clampedStart("PPO", startIdx, ppoLookback(optInFastPeriod, optInSlowPeriod, optInMAType));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("PPO", "inReal", inReal, guardInLen);
      requireLength("PPO", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = ppoImpl(startIdx, endIdx, inReal, optInFastPeriod, optInSlowPeriod, optInMAType, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.SUCCESS ) {
         throw failure("PPO", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live PPO stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#ppo} over the same series.
    * Open with {@link Core#ppoOpen}; there is no close — the handle is
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
   public static final class PpoStream {
      private Core core;
      private int optInFastPeriod;
      private int optInSlowPeriod;
      private MAType optInMAType;
      private double cur_outReal;
      private int slowLookback;
      private int windowed;
      private int zeroRun;
      private MaStream sub0;
      private MaStream sub1;
      private int outRangeBegIdx;
      private int outRangeCount;

      private PpoStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#ppo} reports over the same bars: the
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
            throw failure("PPO advance", RetCode.OUT_OF_RANGE_END_INDEX);
         this.outRangeCount++;
      }

      private PpoStream( PpoStream other ) {
         this.core = other.core;
         this.optInFastPeriod = other.optInFastPeriod;
         this.optInSlowPeriod = other.optInSlowPeriod;
         this.optInMAType = other.optInMAType;
         this.cur_outReal = other.cur_outReal;
         this.slowLookback = other.slowLookback;
         this.windowed = other.windowed;
         this.zeroRun = other.zeroRun;
         this.sub0 = new MaStream(other.sub0);
         this.sub1 = new MaStream(other.sub1);
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
            throw failure("PPO update", RetCode.OUT_OF_RANGE_END_INDEX);
         if( !Double.isFinite(inReal) )
            throw nonFiniteBar("PPO update", "inReal");
         core.ppoStepImpl(this, inReal);
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
            throw nonFiniteBar("PPO peek", "inReal");
         PpoStream sp = this;
         double tempReal = 0.0;
         double cur_tempBuffer = 0.0;
         double cur_outReal = 0.0;
         /* Pipeline the new bar through the sub-streams (batch tail order). */
         cur_tempBuffer = sp.sub0.peek(inReal);
         cur_outReal = sp.sub1.peek(inReal);
         int slowLookback = sp.slowLookback;
         int windowed = sp.windowed;
         int zeroRun = sp.zeroRun;
         /* Combine map (batch tail, per bar). */
         if( windowed != 0 ) {
            zeroRun = (Math.abs(inReal) <= 0.0) ? zeroRun + 1 : 0;
            tempReal = cur_outReal;
            if( zeroRun > slowLookback ) {
               zeroRun = slowLookback;
               cur_outReal = 0.0;
            } else if( !((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001)) ) {
               cur_outReal = (cur_tempBuffer - tempReal) / tempReal * 100.0;
            } else {
               cur_outReal = 0.0;
            }
         } else {
            /* Calculate ((fast MA)-(slow MA))/(slow MA) in the output. */
            tempReal = cur_outReal;
            if( !((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001)) ) {
               cur_outReal = (cur_tempBuffer - tempReal) / tempReal * 100.0;
            } else {
               cur_outReal = 0.0;
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
      public PpoStream clone() {
         return new PpoStream(this);
      }
   }
   private void ppoStepImpl( PpoStream sp, double inReal )
   {
      double tempReal = 0.0;
      double cur_tempBuffer = 0.0;
      double cur_outReal = 0.0;
      /* Pipeline the new bar through the sub-streams (batch tail order). */
      cur_tempBuffer = sp.sub0.update(inReal);
      cur_outReal = sp.sub1.update(inReal);
      int slowLookback = sp.slowLookback;
      int windowed = sp.windowed;
      int zeroRun = sp.zeroRun;
      /* Combine map (batch tail, per bar). */
      if( windowed != 0 ) {
         zeroRun = (Math.abs(inReal) <= 0.0) ? zeroRun + 1 : 0;
         tempReal = cur_outReal;
         if( zeroRun > slowLookback ) {
            zeroRun = slowLookback;
            cur_outReal = 0.0;
         } else if( !((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001)) ) {
            cur_outReal = (cur_tempBuffer - tempReal) / tempReal * 100.0;
         } else {
            cur_outReal = 0.0;
         }
      } else {
         /* Calculate ((fast MA)-(slow MA))/(slow MA) in the output. */
         tempReal = cur_outReal;
         if( !((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001)) ) {
            cur_outReal = (cur_tempBuffer - tempReal) / tempReal * 100.0;
         } else {
            cur_outReal = 0.0;
         }
      }
      sp.zeroRun = zeroRun;
      sp.cur_outReal = cur_outReal;
   }
   private RetCode ppoOpenImpl( PpoStream sp, double inReal[], int startIdx, int optInFastPeriod, int optInSlowPeriod, MAType optInMAType, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
   {
      double[] tempBuffer;
      RetCode retCode;
      double tempReal = 0;
      int tempInteger = 0;
      MInteger fastBeg = new MInteger();
      MInteger fastNb = new MInteger();
      int offset = 0;
      int slowLookback = 0;
      int windowed = 0;
      int zeroRun = 0;
      int i = 0;
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.OUT_OF_RANGE_START_INDEX;
      }
      if( historyLen > INDEX_MAX + 1 ) {
         return RetCode.OUT_OF_RANGE_END_INDEX;
      }
      if( optInFastPeriod == Integer.MIN_VALUE ) {
         optInFastPeriod = 12;
      } else if( optInFastPeriod < 2 || optInFastPeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInSlowPeriod == Integer.MIN_VALUE ) {
         optInSlowPeriod = 26;
      } else if( optInSlowPeriod < 2 || optInSlowPeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInMAType == MAType.DEFAULT ) {
         optInMAType = MAType.EMA;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.INSUFFICIENT_HISTORY;
      }
      if( historyLen < ppoLookback(optInFastPeriod, optInSlowPeriod, optInMAType) + 1 ) {
         return RetCode.INSUFFICIENT_HISTORY;
      }
      double[] sc_outReal = outStride == 1 ? outReal : new double[historyLen];
      /* Nothing to produce: the range ends before the lookback. Return before
       * touching anything.
       *
       * Without this the fast MA below runs first, and its lookback is SMALLER
       * than ppo's own — so it reads the whole range and computes a result the
       * empty slow MA then discards. Observably identical (the slow MA's own early
       * return already yields 0,0 here), but it is the difference between "a range
       * that ends before the lookback reads nothing" being true of this function and
       * being false: with a caller-supplied inReal that stops short of endIdx, that
       * discarded work is an out-of-bounds read. Pinned by the zero-length no-I/O
       * probe over every guarded core.
       */
      if( maLookback(Math.max(optInSlowPeriod, optInFastPeriod), optInMAType) > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.INSUFFICIENT_HISTORY ;
      }
      /* Allocate an intermediate buffer. */
      tempBuffer = new double[(int)((endIdx - startIdx + 1) * 1)];
      /* Make sure slow is really slower than
       * the fast period! if not, swap...
       */
      if( optInSlowPeriod < optInFastPeriod ) {
         /* swap */
         tempInteger = optInSlowPeriod;
         optInSlowPeriod = optInFastPeriod;
         optInFastPeriod = tempInteger;
      }
      /* Calculate the fast MA into the tempBuffer. */
      /* Sub-stream 0: ma over `inReal`, warmed from bar 0 up to the
       * sub-call's own startIdx (the seeding point). */
      MaStream sub0 = maOpenAndFillInternal(inReal, startIdx, optInFastPeriod, optInMAType, fastBeg, fastNb, tempBuffer);
      retCode = RetCode.SUCCESS;
      /* Calculate the slow MA into the output. */
      /* Sub-stream 1: ma over `inReal`, warmed from bar 0 up to the
       * sub-call's own startIdx (the seeding point). */
      MaStream sub1 = maOpenAndFillInternal(inReal, startIdx, optInSlowPeriod, optInMAType, outBegIdx, outNBElement, sc_outReal);
      retCode = RetCode.SUCCESS;
      /* fastNb - *outNBElement == slowBeg - fastBeg (the fast MA has at least as
       * many outputs), so tempBuffer[i+offset] is the fast MA at the same bar as
       * outReal[i], with a non-negative index. An empty slow MA skips the loop.
       */
      offset = fastNb.value - outNBElement.value;
      /* A windowed slow MA (SMA, WMA, TRIMA, HMA) over bars that are all exactly
       * zero is exactly zero, but its running sums leave residue there that
       * TA_IS_ZERO does not catch, and residue over residue is noise where 0 is
       * documented. zeroRun counts the trailing zero bars, held at slowLookback once
       * the window is dead. The recursive MA types really are nonzero on such a
       * window, so they keep the plain loop.
       */
      slowLookback = maLookback(optInSlowPeriod, optInMAType);
      windowed = (optInMAType == MAType.SMA || optInMAType == MAType.WMA || optInMAType == MAType.TRIMA || optInMAType == MAType.HMA) ? 1 : 0;
      zeroRun = 0;
      for( i = outBegIdx.value - slowLookback; i < outBegIdx.value; i += 1 ) {
         zeroRun = (Math.abs(inReal[i]) <= 0.0) ? zeroRun + 1 : 0;
      }
      if( windowed != 0 ) {
         for( i = 0; i < (int)outNBElement.value; i += 1 ) {
            zeroRun = (Math.abs(inReal[outBegIdx.value + i]) <= 0.0) ? zeroRun + 1 : 0;
            tempReal = sc_outReal[i];
            if( zeroRun > slowLookback ) {
               zeroRun = slowLookback;
               sc_outReal[i] = 0.0;
            } else if( !((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001)) ) {
               sc_outReal[i] = (tempBuffer[i + offset] - tempReal) / tempReal * 100.0;
            } else {
               sc_outReal[i] = 0.0;
            }
         }
      } else {
         /* Calculate ((fast MA)-(slow MA))/(slow MA) in the output. */
         for( i = 0; i < (int)outNBElement.value; i += 1 ) {
            tempReal = sc_outReal[i];
            if( !((-0.00000000000001 < tempReal) && (tempReal < 0.00000000000001)) ) {
               sc_outReal[i] = (tempBuffer[i + offset] - tempReal) / tempReal * 100.0;
            } else {
               sc_outReal[i] = 0.0;
            }
         }
      }
      /* Capture the live producer state + sub handles. */
      if( outNBElement.value < 1 ) {
         return RetCode.INSUFFICIENT_HISTORY;
      }
      sp.optInFastPeriod = optInFastPeriod;
      sp.optInSlowPeriod = optInSlowPeriod;
      sp.optInMAType = optInMAType;
      sp.sub0 = sub0;
      sp.sub1 = sub1;
      sp.slowLookback = slowLookback;
      sp.windowed = windowed;
      sp.zeroRun = zeroRun;
      sp.cur_outReal = sc_outReal[outNBElement.value - 1];
      return RetCode.SUCCESS;
   }
   /* ppoOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   PpoStream ppoOpenAndFillInternal( double inReal[], int startIdx, int optInFastPeriod, int optInSlowPeriod, MAType optInMAType, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      PpoStream sp = new PpoStream(this);
      RetCode retCode = ppoOpenImpl(sp, inReal, startIdx, optInFastPeriod, optInSlowPeriod, optInMAType, outBegIdx, outNBElement, outReal, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.SUCCESS ) {
         return sp;
      }
      if( retCode == RetCode.INSUFFICIENT_HISTORY ) {
         throw insufficientHistory("PPO openAndFill", inReal.length, startIdx, ppoLookback(optInFastPeriod, optInSlowPeriod, optInMAType));
      }
      throw streamFailure("PPO openAndFill", retCode);
   }
   /* Internal startIdx-anchored open behind ppoOpen (composition seam). */
   PpoStream ppoOpenInternal( double inReal[], int startIdx, int optInFastPeriod, int optInSlowPeriod, MAType optInMAType )
   {
      PpoStream sp = new PpoStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      RetCode retCode = ppoOpenImpl(sp, inReal, startIdx, optInFastPeriod, optInSlowPeriod, optInMAType, outBegIdx, outNBElement, sink_outReal, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.SUCCESS ) {
         return sp;
      }
      if( retCode == RetCode.INSUFFICIENT_HISTORY ) {
         throw insufficientHistory("PPO open", inReal.length, startIdx, ppoLookback(optInFastPeriod, optInSlowPeriod, optInMAType));
      }
      throw streamFailure("PPO open", retCode);
   }
   /**
    * Open a live PPO stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#ppo} at that bar.
    * <p>The history must hold at least {@code ppoLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@link Integer#MIN_VALUE} and {@link MAType#DEFAULT} select a
    * parameter's documented default, as in the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public PpoStream ppoOpen( double inReal[], int optInFastPeriod, int optInSlowPeriod, MAType optInMAType )
   {
      requireArgument("PPO open", "inReal", inReal);
      requireHistory("PPO open", inReal.length);
      requireArgument("PPO open", "optInMAType", optInMAType);
      return ppoOpenInternal(inReal, 0, optInFastPeriod, optInSlowPeriod, optInMAType);
   }
   /**
    * {@link Core#ppoOpen} that also fills the output array(s) bit-identically
    * to {@link Core#ppo} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link PpoStream#outRange()}.
    */
   public PpoStream ppoOpenAndFill( double inReal[], int optInFastPeriod, int optInSlowPeriod, MAType optInMAType, double outReal[] )
   {
      requireArgument("PPO openAndFill", "inReal", inReal);
      requireHistory("PPO openAndFill", inReal.length);
      requireArgument("PPO openAndFill", "optInMAType", optInMAType);
      int guardOutLen = openFillCount("PPO openAndFill", inReal.length, ppoLookback(optInFastPeriod, optInSlowPeriod, optInMAType));
      requireLength("PPO openAndFill", "outReal", outReal, guardOutLen);
      if( (Object)outReal == (Object)inReal ) {
         throw streamFailure("PPO openAndFill", RetCode.BAD_PARAM);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return ppoOpenAndFillInternal(inReal, 0, optInFastPeriod, optInSlowPeriod, optInMAType, outBegIdx, outNBElement, outReal);
   }
