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
 *  092526 MF,CC  First version (issue #449).
 */

/* Using percentb_ALT1 for TA_ALT={BATCH,JAVA} */

   /**
    * Number of leading input bars {@link Core#percentb} consumes before it can
    * produce its first value.
    * <p>Equivalently, the index of the first bar with a value when the whole
    * series is requested. Feed at least {@code lookback + 1} bars to get any
    * output.
    *
    * @param optInTimePeriod Periods for the MA and standard deviation (default
    *        20; range 2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInNbDevUp Standard-deviation multiplier for the upper band
    *        (default 2; {@link Core#REAL_DEFAULT} selects the default).
    * @param optInNbDevDn Standard-deviation multiplier for the lower band
    *        (default 2; {@link Core#REAL_DEFAULT} selects the default).
    * @param optInMAType Moving-average type for the middle band (default 0 =
    *        SMA; values: 0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA,
    *        8=T3, 9=HMA, 10=DISABLED, 11=DEFAULT, 12=ZLEMA, 13=RMA;
    *        {@code MAType.DEFAULT} selects the default).
    * @return The lookback, or {@code -1} if a parameter is out of range.
    */
   public int percentbLookback( int optInTimePeriod, double optInNbDevUp, double optInNbDevDn, MAType optInMAType )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 20;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return -1;
      }
      if( optInNbDevUp == REAL_DEFAULT ) {
         optInNbDevUp = 2e0;
      } else if( !(optInNbDevUp >= REAL_MIN && optInNbDevUp <= REAL_MAX) ) {
         return -1;
      }
      if( optInNbDevDn == REAL_DEFAULT ) {
         optInNbDevDn = 2e0;
      } else if( !(optInNbDevDn >= REAL_MIN && optInNbDevDn <= REAL_MAX) ) {
         return -1;
      }
      if( optInMAType == MAType.DEFAULT ) {
         optInMAType = MAType.SMA;
      }
      return bbandsLookback(optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType) ;

   }
   RetCode percentbImpl( int startIdx,
                         int endIdx,
                         double inReal[],
                         int optInTimePeriod,
                         double optInNbDevUp,
                         double optInNbDevDn,
                         MAType optInMAType,
                         MInteger outBegIdx,
                         MInteger outNBElement,
                         double outReal[] )
   {
      RetCode retCode;
      int i = 0;
      MInteger maBegIdx = new MInteger();
      MInteger maNbElement = new MInteger();
      MInteger xBegIdx = new MInteger();
      MInteger xNbElement = new MInteger();
      int offsetMA = 0;
      int offsetX = 0;
      double middle = 0;
      double deviation = 0;
      double tempReal = 0;
      double upper = 0;
      double lower = 0;
      double den = 0;
      double[] tempMA;
      double[] tempX;
      if( (startIdx < 0) || (startIdx > INDEX_MAX) ) {
         return RetCode.OUT_OF_RANGE_START_INDEX ;
      }
      if( (endIdx < 0) || (endIdx > INDEX_MAX) || (endIdx < startIdx)) {
         return RetCode.OUT_OF_RANGE_END_INDEX ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 20;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInNbDevUp == REAL_DEFAULT ) {
         optInNbDevUp = 2e0;
      } else if( !(optInNbDevUp >= REAL_MIN && optInNbDevUp <= REAL_MAX) ) {
         return RetCode.BAD_PARAM;
      }
      if( optInNbDevDn == REAL_DEFAULT ) {
         optInNbDevDn = 2e0;
      } else if( !(optInNbDevDn >= REAL_MIN && optInNbDevDn <= REAL_MAX) ) {
         return RetCode.BAD_PARAM;
      }
      if( optInMAType == MAType.DEFAULT ) {
         optInMAType = MAType.SMA;
      }
      if( optInMAType == MAType.SMA ) {
         double[] _mid = new double[256];
         double[] _var = new double[256];
         double[] _x = new double[256];
         double maTotal;
         double shift;
         double varTotal1;
         double varTotal2;
         double meanValue1;
         double variance;
         double _invPeriod;
         double _tempReal;
         double _peakTotal2;
         int _i;
         int _j;
         int _k;
         int _t;
         int _outIdx;
         int _tileEnd;
         int _trailingIdx;
         int _windowStart;
         int _lookbackTotal;
         int _barsSinceReseed;
         _lookbackTotal = optInTimePeriod - 1;
         if( startIdx < _lookbackTotal ) {
            startIdx = _lookbackTotal;
         }
         if( startIdx > endIdx ) {
            outBegIdx.value = 0;
            outNBElement.value = 0;
            return RetCode.SUCCESS ;
         }
         _invPeriod = 1.0 / (double)optInTimePeriod;
         _trailingIdx = startIdx - _lookbackTotal;
         shift = inReal[_trailingIdx];
         maTotal = 0.0;
         varTotal1 = 0.0;
         varTotal2 = 0.0;
         for( _j = _trailingIdx; _j < startIdx; _j += 1 ) {
            maTotal += inReal[_j];
            _tempReal = inReal[_j] - shift;
            varTotal1 += _tempReal;
            _tempReal *= _tempReal;
            varTotal2 += _tempReal;
         }
         _i = startIdx;
         _outIdx = 0;
         _barsSinceReseed = 32 * optInTimePeriod;
         _peakTotal2 = varTotal2;
         do {
            if( endIdx - _i > 255 ) {
               _tileEnd = _i + 255;
            } else {
               _tileEnd = endIdx;
            }
            _t = 0;
            do {
               _x[_t] = inReal[_i];
               maTotal += inReal[_i];
               _tempReal = inReal[_i] - shift;
               varTotal1 += _tempReal;
               _tempReal *= _tempReal;
               varTotal2 += _tempReal;
               _peakTotal2 = (varTotal2 > _peakTotal2) ? varTotal2 : _peakTotal2;
               meanValue1 = varTotal1 * _invPeriod;
               variance = varTotal2 * _invPeriod - meanValue1 * meanValue1;
               _mid[_t] = maTotal / optInTimePeriod;
               maTotal -= inReal[_trailingIdx];
               _tempReal = inReal[_trailingIdx] - shift;
               varTotal1 -= _tempReal;
               _tempReal *= _tempReal;
               varTotal2 -= _tempReal;
               _trailingIdx += 1;
               _barsSinceReseed -= 1;
               if( variance < 0.000001 * (_peakTotal2 * _invPeriod) || _barsSinceReseed <= 0 ) {
                  _barsSinceReseed = 32 * optInTimePeriod;
                  _windowStart = _i - _lookbackTotal;
                  _tempReal = 0.0;
                  for( _j = _windowStart; _j <= _i; _j += 1 ) {
                     _tempReal += inReal[_j];
                  }
                  shift = _tempReal * _invPeriod;
                  varTotal1 = 0.0;
                  varTotal2 = 0.0;
                  for( _j = _windowStart; _j <= _i; _j += 1 ) {
                     _tempReal = inReal[_j] - shift;
                     varTotal1 += _tempReal;
                     _tempReal *= _tempReal;
                     varTotal2 += _tempReal;
                  }
                  meanValue1 = varTotal1 * _invPeriod;
                  variance = varTotal2 * _invPeriod - meanValue1 * meanValue1;
                  if( variance < 0.000001 * (varTotal2 * _invPeriod) ) {
                     shift = inReal[_i];
                     varTotal1 = 0.0;
                     varTotal2 = 0.0;
                     for( _j = _windowStart; _j <= _i; _j += 1 ) {
                        _tempReal = inReal[_j] - shift;
                        varTotal1 += _tempReal;
                        _tempReal *= _tempReal;
                        varTotal2 += _tempReal;
                     }
                     meanValue1 = varTotal1 * _invPeriod;
                     variance = varTotal2 * _invPeriod - meanValue1 * meanValue1;
                  }
                  _peakTotal2 = varTotal2;
                  if( variance < 0.000000000001 * (varTotal2 * _invPeriod) ) {
                     variance = 0.0;
                  }
                  _tempReal = inReal[_windowStart] - shift;
                  varTotal1 -= _tempReal;
                  _tempReal *= _tempReal;
                  varTotal2 -= _tempReal;
               }
               _var[_t] = variance;
               _t += 1;
               _i += 1;
            } while( _i <= _tileEnd );
            if( optInNbDevUp == optInNbDevDn ) {
               for( _k = 0; _k < _t; _k += 1 ) {
                  middle = _mid[_k];
                  tempReal = Math.sqrt(_var[_k]) * optInNbDevUp;
                  upper = tempReal + middle;
                  lower = middle - tempReal;
                  den = upper - lower;
                  _var[_k] = (_x[_k] - lower) / den;
                  _mid[_k] = den;
               }
            } else {
               for( _k = 0; _k < _t; _k += 1 ) {
                  middle = _mid[_k];
                  deviation = Math.sqrt(_var[_k]);
                  upper = Math.fma(deviation, optInNbDevUp, middle);
                  lower = middle - deviation * optInNbDevDn;
                  den = upper - lower;
                  _var[_k] = (_x[_k] - lower) / den;
                  _mid[_k] = den;
               }
            }
            for( _k = 0; _k < _t; _k += 1 ) {
               if( _mid[_k] == 0.0 ) {
                  _var[_k] = 0.5;
               }
            }
            /* outReal may be inReal: a tile is written only after its last read. */
            System.arraycopy(_var, 0, outReal, _outIdx, _t * 1);
            _outIdx += _t;
         } while( _i <= endIdx );
         outNBElement.value = _outIdx;
         outBegIdx.value = startIdx;
         return RetCode.SUCCESS ;
      }
      if( percentbLookback(optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType) > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.SUCCESS ;
      }
      tempMA = new double[(int)((endIdx - startIdx + 1) * 1)];
      tempX = new double[(int)((endIdx - startIdx + 1) * 1)];
      OutRange _xr0 = ma(startIdx, endIdx, inReal, optInTimePeriod, optInMAType, tempMA);
      maBegIdx.value = _xr0.begIdx();
      maNbElement.value = _xr0.count();
      retCode = RetCode.SUCCESS;
      OutRange _xr1 = ma(maBegIdx.value, endIdx, inReal, 1, optInMAType, tempX);
      xBegIdx.value = _xr1.begIdx();
      xNbElement.value = _xr1.count();
      retCode = RetCode.SUCCESS;
      OutRange _xr2 = var(maBegIdx.value, endIdx, inReal, optInTimePeriod, 1.0, outReal);
      outBegIdx.value = _xr2.begIdx();
      outNBElement.value = _xr2.count();
      retCode = RetCode.SUCCESS;
      offsetMA = maNbElement.value - outNBElement.value;
      if( offsetMA != 0 ) {
         System.arraycopy(tempMA, offsetMA, tempMA, 0, outNBElement.value * 1);
      }
      offsetX = xNbElement.value - outNBElement.value;
      if( offsetX != 0 ) {
         System.arraycopy(tempX, offsetX, tempX, 0, outNBElement.value * 1);
      }
      if( optInNbDevUp == optInNbDevDn ) {
         for( i = 0; i < (int)outNBElement.value; i += 1 ) {
            middle = tempMA[i];
            tempReal = Math.sqrt(outReal[i]) * optInNbDevUp;
            upper = tempReal + middle;
            lower = middle - tempReal;
            den = upper - lower;
            outReal[i] = (tempX[i] - lower) / den;
            tempMA[i] = den;
         }
      } else {
         for( i = 0; i < (int)outNBElement.value; i += 1 ) {
            middle = tempMA[i];
            deviation = Math.sqrt(outReal[i]);
            upper = Math.fma(deviation, optInNbDevUp, middle);
            lower = middle - deviation * optInNbDevDn;
            den = upper - lower;
            outReal[i] = (tempX[i] - lower) / den;
            tempMA[i] = den;
         }
      }
      for( i = 0; i < (int)outNBElement.value; i += 1 ) {
         if( tempMA[i] == 0.0 ) {
            outReal[i] = 0.5;
         }
      }
      return RetCode.SUCCESS ;
   }
   RetCode percentbImpl( int startIdx,
                         int endIdx,
                         float inReal[],
                         int optInTimePeriod,
                         double optInNbDevUp,
                         double optInNbDevDn,
                         MAType optInMAType,
                         MInteger outBegIdx,
                         MInteger outNBElement,
                         double outReal[] )
   {
      RetCode retCode;
      int i = 0;
      MInteger maBegIdx = new MInteger();
      MInteger maNbElement = new MInteger();
      MInteger xBegIdx = new MInteger();
      MInteger xNbElement = new MInteger();
      int offsetMA = 0;
      int offsetX = 0;
      double middle = 0;
      double deviation = 0;
      double tempReal = 0;
      double upper = 0;
      double lower = 0;
      double den = 0;
      double[] tempMA;
      double[] tempX;
      if( (startIdx < 0) || (startIdx > INDEX_MAX) ) {
         return RetCode.OUT_OF_RANGE_START_INDEX ;
      }
      if( (endIdx < 0) || (endIdx > INDEX_MAX) || (endIdx < startIdx)) {
         return RetCode.OUT_OF_RANGE_END_INDEX ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 20;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInNbDevUp == REAL_DEFAULT ) {
         optInNbDevUp = 2e0;
      } else if( !(optInNbDevUp >= REAL_MIN && optInNbDevUp <= REAL_MAX) ) {
         return RetCode.BAD_PARAM;
      }
      if( optInNbDevDn == REAL_DEFAULT ) {
         optInNbDevDn = 2e0;
      } else if( !(optInNbDevDn >= REAL_MIN && optInNbDevDn <= REAL_MAX) ) {
         return RetCode.BAD_PARAM;
      }
      if( optInMAType == MAType.DEFAULT ) {
         optInMAType = MAType.SMA;
      }
      if( optInMAType == MAType.SMA ) {
         double[] _mid = new double[256];
         double[] _var = new double[256];
         double[] _x = new double[256];
         double maTotal;
         double shift;
         double varTotal1;
         double varTotal2;
         double meanValue1;
         double variance;
         double _invPeriod;
         double _tempReal;
         double _peakTotal2;
         int _i;
         int _j;
         int _k;
         int _t;
         int _outIdx;
         int _tileEnd;
         int _trailingIdx;
         int _windowStart;
         int _lookbackTotal;
         int _barsSinceReseed;
         _lookbackTotal = optInTimePeriod - 1;
         if( startIdx < _lookbackTotal ) {
            startIdx = _lookbackTotal;
         }
         if( startIdx > endIdx ) {
            outBegIdx.value = 0;
            outNBElement.value = 0;
            return RetCode.SUCCESS ;
         }
         _invPeriod = 1.0 / (double)optInTimePeriod;
         _trailingIdx = startIdx - _lookbackTotal;
         shift = (double)inReal[_trailingIdx];
         maTotal = 0.0;
         varTotal1 = 0.0;
         varTotal2 = 0.0;
         for( _j = _trailingIdx; _j < startIdx; _j += 1 ) {
            maTotal += (double)inReal[_j];
            _tempReal = (double)inReal[_j] - shift;
            varTotal1 += _tempReal;
            _tempReal *= _tempReal;
            varTotal2 += _tempReal;
         }
         _i = startIdx;
         _outIdx = 0;
         _barsSinceReseed = 32 * optInTimePeriod;
         _peakTotal2 = varTotal2;
         do {
            if( endIdx - _i > 255 ) {
               _tileEnd = _i + 255;
            } else {
               _tileEnd = endIdx;
            }
            _t = 0;
            do {
               _x[_t] = (double)inReal[_i];
               maTotal += (double)inReal[_i];
               _tempReal = (double)inReal[_i] - shift;
               varTotal1 += _tempReal;
               _tempReal *= _tempReal;
               varTotal2 += _tempReal;
               _peakTotal2 = (varTotal2 > _peakTotal2) ? varTotal2 : _peakTotal2;
               meanValue1 = varTotal1 * _invPeriod;
               variance = varTotal2 * _invPeriod - meanValue1 * meanValue1;
               _mid[_t] = maTotal / optInTimePeriod;
               maTotal -= (double)inReal[_trailingIdx];
               _tempReal = (double)inReal[_trailingIdx] - shift;
               varTotal1 -= _tempReal;
               _tempReal *= _tempReal;
               varTotal2 -= _tempReal;
               _trailingIdx += 1;
               _barsSinceReseed -= 1;
               if( variance < 0.000001 * (_peakTotal2 * _invPeriod) || _barsSinceReseed <= 0 ) {
                  _barsSinceReseed = 32 * optInTimePeriod;
                  _windowStart = _i - _lookbackTotal;
                  _tempReal = 0.0;
                  for( _j = _windowStart; _j <= _i; _j += 1 ) {
                     _tempReal += (double)inReal[_j];
                  }
                  shift = _tempReal * _invPeriod;
                  varTotal1 = 0.0;
                  varTotal2 = 0.0;
                  for( _j = _windowStart; _j <= _i; _j += 1 ) {
                     _tempReal = (double)inReal[_j] - shift;
                     varTotal1 += _tempReal;
                     _tempReal *= _tempReal;
                     varTotal2 += _tempReal;
                  }
                  meanValue1 = varTotal1 * _invPeriod;
                  variance = varTotal2 * _invPeriod - meanValue1 * meanValue1;
                  if( variance < 0.000001 * (varTotal2 * _invPeriod) ) {
                     shift = (double)inReal[_i];
                     varTotal1 = 0.0;
                     varTotal2 = 0.0;
                     for( _j = _windowStart; _j <= _i; _j += 1 ) {
                        _tempReal = (double)inReal[_j] - shift;
                        varTotal1 += _tempReal;
                        _tempReal *= _tempReal;
                        varTotal2 += _tempReal;
                     }
                     meanValue1 = varTotal1 * _invPeriod;
                     variance = varTotal2 * _invPeriod - meanValue1 * meanValue1;
                  }
                  _peakTotal2 = varTotal2;
                  if( variance < 0.000000000001 * (varTotal2 * _invPeriod) ) {
                     variance = 0.0;
                  }
                  _tempReal = (double)inReal[_windowStart] - shift;
                  varTotal1 -= _tempReal;
                  _tempReal *= _tempReal;
                  varTotal2 -= _tempReal;
               }
               _var[_t] = variance;
               _t += 1;
               _i += 1;
            } while( _i <= _tileEnd );
            if( optInNbDevUp == optInNbDevDn ) {
               for( _k = 0; _k < _t; _k += 1 ) {
                  middle = _mid[_k];
                  tempReal = Math.sqrt(_var[_k]) * optInNbDevUp;
                  upper = tempReal + middle;
                  lower = middle - tempReal;
                  den = upper - lower;
                  _var[_k] = (_x[_k] - lower) / den;
                  _mid[_k] = den;
               }
            } else {
               for( _k = 0; _k < _t; _k += 1 ) {
                  middle = _mid[_k];
                  deviation = Math.sqrt(_var[_k]);
                  upper = Math.fma(deviation, optInNbDevUp, middle);
                  lower = middle - deviation * optInNbDevDn;
                  den = upper - lower;
                  _var[_k] = (_x[_k] - lower) / den;
                  _mid[_k] = den;
               }
            }
            for( _k = 0; _k < _t; _k += 1 ) {
               if( _mid[_k] == 0.0 ) {
                  _var[_k] = 0.5;
               }
            }
            System.arraycopy(_var, 0, outReal, _outIdx, _t * 1);
            _outIdx += _t;
         } while( _i <= endIdx );
         outNBElement.value = _outIdx;
         outBegIdx.value = startIdx;
         return RetCode.SUCCESS ;
      }
      if( percentbLookback(optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType) > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.SUCCESS ;
      }
      tempMA = new double[(int)((endIdx - startIdx + 1) * 1)];
      tempX = new double[(int)((endIdx - startIdx + 1) * 1)];
      OutRange _xr0 = ma(startIdx, endIdx, inReal, optInTimePeriod, optInMAType, tempMA);
      maBegIdx.value = _xr0.begIdx();
      maNbElement.value = _xr0.count();
      retCode = RetCode.SUCCESS;
      OutRange _xr1 = ma(maBegIdx.value, endIdx, inReal, 1, optInMAType, tempX);
      xBegIdx.value = _xr1.begIdx();
      xNbElement.value = _xr1.count();
      retCode = RetCode.SUCCESS;
      OutRange _xr2 = var(maBegIdx.value, endIdx, inReal, optInTimePeriod, 1.0, outReal);
      outBegIdx.value = _xr2.begIdx();
      outNBElement.value = _xr2.count();
      retCode = RetCode.SUCCESS;
      offsetMA = maNbElement.value - outNBElement.value;
      if( offsetMA != 0 ) {
         System.arraycopy(tempMA, offsetMA, tempMA, 0, outNBElement.value * 1);
      }
      offsetX = xNbElement.value - outNBElement.value;
      if( offsetX != 0 ) {
         System.arraycopy(tempX, offsetX, tempX, 0, outNBElement.value * 1);
      }
      if( optInNbDevUp == optInNbDevDn ) {
         for( i = 0; i < (int)outNBElement.value; i += 1 ) {
            middle = tempMA[i];
            tempReal = Math.sqrt(outReal[i]) * optInNbDevUp;
            upper = tempReal + middle;
            lower = middle - tempReal;
            den = upper - lower;
            outReal[i] = (tempX[i] - lower) / den;
            tempMA[i] = den;
         }
      } else {
         for( i = 0; i < (int)outNBElement.value; i += 1 ) {
            middle = tempMA[i];
            deviation = Math.sqrt(outReal[i]);
            upper = Math.fma(deviation, optInNbDevUp, middle);
            lower = middle - deviation * optInNbDevDn;
            den = upper - lower;
            outReal[i] = (tempX[i] - lower) / den;
            tempMA[i] = den;
         }
      }
      for( i = 0; i < (int)outNBElement.value; i += 1 ) {
         if( tempMA[i] == 0.0 ) {
            outReal[i] = 0.5;
         }
      }
      return RetCode.SUCCESS ;
   }
   /**
    * Bollinger Bands %B: where the input sits relative to its Bollinger Bands,
    * 0 at the lower band and 1 at the upper band. Values below 0 or above 1
    * mean the input is outside the bands.
    * <p>Formula and more info at <a
    * href="https://ta-lib.org/functions/percentb">ta-lib.org/functions/percentb</a>.
    * <p><b>Notes</b>
    * <ul>
    * <li>An input on the middle band reads {@code optInNbDevDn / (optInNbDevUp + optInNbDevDn)}: 0.5 with equal multipliers.</li>
    * <li>With a simple moving average and both multipliers equal to k, %B = 0.5 + z / (2k), where z is the z-score of the input in its window.</li>
    * <li>The result is a ratio; multiply by 100 to read it as a percentage.</li>
    * <li>Any {@code optInMAType} other than SMA is a TA-Lib generalisation, as it is for BBANDS: the deviation stays the population standard deviation about the simple mean.</li>
    * <li>PERCENTB is bit for bit {@code (inReal - lower) / (upper - lower)} computed from BBANDS' own outputs, and 0.5 wherever those two bands are equal.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range that ends before {@link Core#percentbLookback} is a <b>success
    * with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Input data series.
    * @param optInTimePeriod Periods for the MA and standard deviation (default
    *        20; range 2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInNbDevUp Standard-deviation multiplier for the upper band
    *        (default 2; {@link Core#REAL_DEFAULT} selects the default).
    * @param optInNbDevDn Standard-deviation multiplier for the lower band
    *        (default 2; {@link Core#REAL_DEFAULT} selects the default).
    * @param optInMAType Moving-average type for the middle band (default 0 =
    *        SMA; values: 0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA,
    *        8=T3, 9=HMA, 10=DISABLED, 11=DEFAULT, 12=ZLEMA, 13=RMA;
    *        {@code MAType.DEFAULT} selects the default).
    * @param outReal Position of the input between the lower band (0) and the
    *        upper band (1) Must hold at least
    *        {@code endIdx - max(startIdx, percentbLookback(...)) + 1} values, the
    *        count the call produces (none when that is not positive).
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
    * @see Core#bbw
    * @see Core#stochf
    */
   public OutRange percentb( int startIdx,
                             int endIdx,
                             double inReal[],
                             int optInTimePeriod,
                             double optInNbDevUp,
                             double optInNbDevDn,
                             MAType optInMAType,
                             double outReal[] )
   {
      requireIndexRange("PERCENTB", startIdx, endIdx);
      requireArgument("PERCENTB", "optInMAType", optInMAType);
      int guardStart = clampedStart("PERCENTB", startIdx, percentbLookback(optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("PERCENTB", "inReal", inReal, guardInLen);
      requireLength("PERCENTB", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = percentbImpl(startIdx, endIdx, inReal, optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.SUCCESS ) {
         throw failure("PERCENTB", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Bollinger Bands %B: where the input sits relative to its Bollinger Bands,
    * 0 at the lower band and 1 at the upper band. Values below 0 or above 1
    * mean the input is outside the bands.
    * <p>Formula and more info at <a
    * href="https://ta-lib.org/functions/percentb">ta-lib.org/functions/percentb</a>.
    * <p><b>Notes</b>
    * <ul>
    * <li>An input on the middle band reads {@code optInNbDevDn / (optInNbDevUp + optInNbDevDn)}: 0.5 with equal multipliers.</li>
    * <li>With a simple moving average and both multipliers equal to k, %B = 0.5 + z / (2k), where z is the z-score of the input in its window.</li>
    * <li>The result is a ratio; multiply by 100 to read it as a percentage.</li>
    * <li>Any {@code optInMAType} other than SMA is a TA-Lib generalisation, as it is for BBANDS: the deviation stays the population standard deviation about the simple mean.</li>
    * <li>PERCENTB is bit for bit {@code (inReal - lower) / (upper - lower)} computed from BBANDS' own outputs, and 0.5 wherever those two bands are equal.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range that ends before {@link Core#percentbLookback} is a <b>success
    * with no values</b> ({@code count() == 0}), not an error.
    *
    * @param startIdx First bar of the requested range (inclusive).
    * @param endIdx Last bar of the requested range (inclusive).
    * @param inReal Input data series.
    * @param optInTimePeriod Periods for the MA and standard deviation (default
    *        20; range 2..100000; {@code Integer.MIN_VALUE} selects the default).
    * @param optInNbDevUp Standard-deviation multiplier for the upper band
    *        (default 2; {@link Core#REAL_DEFAULT} selects the default).
    * @param optInNbDevDn Standard-deviation multiplier for the lower band
    *        (default 2; {@link Core#REAL_DEFAULT} selects the default).
    * @param optInMAType Moving-average type for the middle band (default 0 =
    *        SMA; values: 0=SMA, 1=EMA, 2=WMA, 3=DEMA, 4=TEMA, 5=TRIMA, 6=KAMA, 7=MAMA,
    *        8=T3, 9=HMA, 10=DISABLED, 11=DEFAULT, 12=ZLEMA, 13=RMA;
    *        {@code MAType.DEFAULT} selects the default).
    * @param outReal Position of the input between the lower band (0) and the
    *        upper band (1) Must hold at least
    *        {@code endIdx - max(startIdx, percentbLookback(...)) + 1} values, the
    *        count the call produces (none when that is not positive).
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
    * @see Core#bbw
    * @see Core#stochf
    */
   public OutRange percentb( int startIdx,
                             int endIdx,
                             float inReal[],
                             int optInTimePeriod,
                             double optInNbDevUp,
                             double optInNbDevDn,
                             MAType optInMAType,
                             double outReal[] )
   {
      requireIndexRange("PERCENTB", startIdx, endIdx);
      requireArgument("PERCENTB", "optInMAType", optInMAType);
      int guardStart = clampedStart("PERCENTB", startIdx, percentbLookback(optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("PERCENTB", "inReal", inReal, guardInLen);
      requireLength("PERCENTB", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = percentbImpl(startIdx, endIdx, inReal, optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.SUCCESS ) {
         throw failure("PERCENTB", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

/* Using percentb_ALT2 for TA_ALT={STREAM,ALL_LANGUAGES} */

   /**
    * A live PERCENTB stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#percentb} over the same series.
    * Open with {@link Core#percentbOpen}; there is no close — the handle is
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
   public static final class PercentbStream {
      private Core core;
      private int optInTimePeriod;
      private double optInNbDevUp;
      private double optInNbDevDn;
      private MAType optInMAType;
      private double cur_outReal;
      private MaStream sub0;
      private VarStream sub1;
      private int outRangeBegIdx;
      private int outRangeCount;

      private PercentbStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#percentb} reports over the same bars: the
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
            throw failure("PERCENTB advance", RetCode.OUT_OF_RANGE_END_INDEX);
         this.outRangeCount++;
      }

      private PercentbStream( PercentbStream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.optInNbDevUp = other.optInNbDevUp;
         this.optInNbDevDn = other.optInNbDevDn;
         this.optInMAType = other.optInMAType;
         this.cur_outReal = other.cur_outReal;
         this.sub0 = new MaStream(other.sub0);
         this.sub1 = new VarStream(other.sub1);
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
            throw failure("PERCENTB update", RetCode.OUT_OF_RANGE_END_INDEX);
         if( !Double.isFinite(inReal) )
            throw nonFiniteBar("PERCENTB update", "inReal");
         core.percentbStepImpl(this, inReal);
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
            throw nonFiniteBar("PERCENTB peek", "inReal");
         PercentbStream sp = this;
         double den = 0.0;
         double deviation = 0.0;
         double lower = 0.0;
         double middle = 0.0;
         double tempReal = 0.0;
         double upper = 0.0;
         double cur_tempX = 0.0;
         double cur_tempMA = 0.0;
         double cur_outReal = 0.0;
         cur_tempX = inReal;
         /* Pipeline the new bar through the sub-streams (batch tail order). */
         cur_tempMA = sp.sub0.peek(inReal);
         cur_outReal = sp.sub1.peek(inReal);
         /* Combine map (batch tail, per bar). */
         if( sp.optInNbDevUp == sp.optInNbDevDn ) {
            middle = cur_tempMA;
            tempReal = Math.sqrt(cur_outReal) * sp.optInNbDevUp;
            upper = middle + tempReal;
            lower = middle - tempReal;
            den = upper - lower;
            cur_outReal = (cur_tempX - lower) / den;
            if( den == 0.0 ) {
               cur_outReal = 0.5;
            }
         } else {
            middle = cur_tempMA;
            deviation = Math.sqrt(cur_outReal);
            upper = Math.fma(deviation, sp.optInNbDevUp, middle);
            lower = middle - deviation * sp.optInNbDevDn;
            den = upper - lower;
            cur_outReal = (cur_tempX - lower) / den;
            if( den == 0.0 ) {
               cur_outReal = 0.5;
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
      public PercentbStream clone() {
         return new PercentbStream(this);
      }
   }
   private void percentbStepImpl( PercentbStream sp, double inReal )
   {
      double den = 0.0;
      double deviation = 0.0;
      double lower = 0.0;
      double middle = 0.0;
      double tempReal = 0.0;
      double upper = 0.0;
      double cur_tempX = 0.0;
      double cur_tempMA = 0.0;
      double cur_outReal = 0.0;
      cur_tempX = inReal;
      /* Pipeline the new bar through the sub-streams (batch tail order). */
      cur_tempMA = sp.sub0.update(inReal);
      cur_outReal = sp.sub1.update(inReal);
      /* Combine map (batch tail, per bar). */
      if( sp.optInNbDevUp == sp.optInNbDevDn ) {
         middle = cur_tempMA;
         tempReal = Math.sqrt(cur_outReal) * sp.optInNbDevUp;
         upper = middle + tempReal;
         lower = middle - tempReal;
         den = upper - lower;
         cur_outReal = (cur_tempX - lower) / den;
         if( den == 0.0 ) {
            cur_outReal = 0.5;
         }
      } else {
         middle = cur_tempMA;
         deviation = Math.sqrt(cur_outReal);
         upper = Math.fma(deviation, sp.optInNbDevUp, middle);
         lower = middle - deviation * sp.optInNbDevDn;
         den = upper - lower;
         cur_outReal = (cur_tempX - lower) / den;
         if( den == 0.0 ) {
            cur_outReal = 0.5;
         }
      }
      sp.cur_outReal = cur_outReal;
   }
   private RetCode percentbOpenImpl( PercentbStream sp, double inReal[], int startIdx, int optInTimePeriod, double optInNbDevUp, double optInNbDevDn, MAType optInMAType, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
   {
      RetCode retCode;
      int i = 0;
      int lookbackTotal = 0;
      int firstIdx = 0;
      int today = 0;
      int outIdx = 0;
      MInteger maBegIdx = new MInteger();
      MInteger maNbElement = new MInteger();
      int offset = 0;
      double middle = 0;
      double deviation = 0;
      double tempReal = 0;
      double upper = 0;
      double lower = 0;
      double den = 0;
      double[] tempMA;
      double[] tempX;
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.OUT_OF_RANGE_START_INDEX;
      }
      if( historyLen > INDEX_MAX + 1 ) {
         return RetCode.OUT_OF_RANGE_END_INDEX;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 20;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BAD_PARAM;
      }
      if( optInNbDevUp == REAL_DEFAULT ) {
         optInNbDevUp = 2e0;
      } else if( !(optInNbDevUp >= REAL_MIN && optInNbDevUp <= REAL_MAX) ) {
         return RetCode.BAD_PARAM;
      }
      if( optInNbDevDn == REAL_DEFAULT ) {
         optInNbDevDn = 2e0;
      } else if( !(optInNbDevDn >= REAL_MIN && optInNbDevDn <= REAL_MAX) ) {
         return RetCode.BAD_PARAM;
      }
      if( optInMAType == MAType.DEFAULT ) {
         optInMAType = MAType.SMA;
      }
      if( startIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.INSUFFICIENT_HISTORY;
      }
      if( historyLen < percentbLookback(optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType) + 1 ) {
         return RetCode.INSUFFICIENT_HISTORY;
      }
      double[] sc_outReal = outStride == 1 ? outReal : new double[historyLen];
      lookbackTotal = percentbLookback(optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType);
      firstIdx = startIdx;
      if( firstIdx < lookbackTotal ) {
         firstIdx = lookbackTotal;
      }
      if( firstIdx > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.INSUFFICIENT_HISTORY ;
      }
      tempX = new double[(int)((endIdx - firstIdx + 1) * 1)];
      tempMA = new double[(int)((endIdx - startIdx + 1) * 1)];
      outIdx = 0;
      today = firstIdx;
      while( today <= endIdx ) {
         tempX[outIdx++] = inReal[today];
         today += 1;
      }
      /* Sub-stream 0: ma over `inReal`, warmed from bar 0 up to the
       * sub-call's own startIdx (the seeding point). */
      MaStream sub0 = maOpenAndFillInternal(inReal, startIdx, optInTimePeriod, optInMAType, maBegIdx, maNbElement, tempMA);
      retCode = RetCode.SUCCESS;
      /* Sub-stream 1: var over `inReal`, warmed from bar 0 up to the
       * sub-call's own startIdx (the seeding point). */
      VarStream sub1 = varOpenAndFillInternal(inReal, maBegIdx.value, optInTimePeriod, 1.0, outBegIdx, outNBElement, sc_outReal);
      retCode = RetCode.SUCCESS;
      offset = maNbElement.value - outNBElement.value;
      if( optInNbDevUp == optInNbDevDn ) {
         for( i = 0; i < (int)outNBElement.value; i += 1 ) {
            middle = tempMA[i + offset];
            tempReal = Math.sqrt(sc_outReal[i]) * optInNbDevUp;
            upper = middle + tempReal;
            lower = middle - tempReal;
            den = upper - lower;
            sc_outReal[i] = (tempX[i] - lower) / den;
            if( den == 0.0 ) {
               sc_outReal[i] = 0.5;
            }
         }
      } else {
         for( i = 0; i < (int)outNBElement.value; i += 1 ) {
            middle = tempMA[i + offset];
            deviation = Math.sqrt(sc_outReal[i]);
            upper = Math.fma(deviation, optInNbDevUp, middle);
            lower = middle - deviation * optInNbDevDn;
            den = upper - lower;
            sc_outReal[i] = (tempX[i] - lower) / den;
            if( den == 0.0 ) {
               sc_outReal[i] = 0.5;
            }
         }
      }
      /* Capture the live producer state + sub handles. */
      if( outNBElement.value < 1 ) {
         return RetCode.INSUFFICIENT_HISTORY;
      }
      /* Capture the live batch state into the handle. */
      sp.optInTimePeriod = optInTimePeriod;
      sp.optInNbDevUp = optInNbDevUp;
      sp.optInNbDevDn = optInNbDevDn;
      sp.optInMAType = optInMAType;
      sp.sub0 = sub0;
      sp.sub1 = sub1;
      sp.cur_outReal = sc_outReal[outNBElement.value - 1];
      return RetCode.SUCCESS;
   }
   /* percentbOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   PercentbStream percentbOpenAndFillInternal( double inReal[], int startIdx, int optInTimePeriod, double optInNbDevUp, double optInNbDevDn, MAType optInMAType, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      PercentbStream sp = new PercentbStream(this);
      RetCode retCode = percentbOpenImpl(sp, inReal, startIdx, optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType, outBegIdx, outNBElement, outReal, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.SUCCESS ) {
         return sp;
      }
      if( retCode == RetCode.INSUFFICIENT_HISTORY ) {
         throw insufficientHistory("PERCENTB openAndFill", inReal.length, startIdx, percentbLookback(optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType));
      }
      throw streamFailure("PERCENTB openAndFill", retCode);
   }
   /* Internal startIdx-anchored open behind percentbOpen (composition seam). */
   PercentbStream percentbOpenInternal( double inReal[], int startIdx, int optInTimePeriod, double optInNbDevUp, double optInNbDevDn, MAType optInMAType )
   {
      PercentbStream sp = new PercentbStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      RetCode retCode = percentbOpenImpl(sp, inReal, startIdx, optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType, outBegIdx, outNBElement, sink_outReal, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.SUCCESS ) {
         return sp;
      }
      if( retCode == RetCode.INSUFFICIENT_HISTORY ) {
         throw insufficientHistory("PERCENTB open", inReal.length, startIdx, percentbLookback(optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType));
      }
      throw streamFailure("PERCENTB open", retCode);
   }
   /**
    * Open a live PERCENTB stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#percentb} at that bar.
    * <p>The history must hold at least {@code percentbLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@link Integer#MIN_VALUE}, {@link Core#REAL_DEFAULT} and
    * {@link MAType#DEFAULT} select a parameter's documented default, as in
    * the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public PercentbStream percentbOpen( double inReal[], int optInTimePeriod, double optInNbDevUp, double optInNbDevDn, MAType optInMAType )
   {
      requireArgument("PERCENTB open", "inReal", inReal);
      requireHistory("PERCENTB open", inReal.length);
      requireArgument("PERCENTB open", "optInMAType", optInMAType);
      return percentbOpenInternal(inReal, 0, optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType);
   }
   /**
    * {@link Core#percentbOpen} that also fills the output array(s) bit-identically
    * to {@link Core#percentb} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link PercentbStream#outRange()}.
    */
   public PercentbStream percentbOpenAndFill( double inReal[], int optInTimePeriod, double optInNbDevUp, double optInNbDevDn, MAType optInMAType, double outReal[] )
   {
      requireArgument("PERCENTB openAndFill", "inReal", inReal);
      requireHistory("PERCENTB openAndFill", inReal.length);
      requireArgument("PERCENTB openAndFill", "optInMAType", optInMAType);
      int guardOutLen = openFillCount("PERCENTB openAndFill", inReal.length, percentbLookback(optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType));
      requireLength("PERCENTB openAndFill", "outReal", outReal, guardOutLen);
      if( (Object)outReal == (Object)inReal ) {
         throw streamFailure("PERCENTB openAndFill", RetCode.BAD_PARAM);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return percentbOpenAndFillInternal(inReal, 0, optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType, outBegIdx, outNBElement, outReal);
   }
