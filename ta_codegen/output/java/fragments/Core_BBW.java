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
 *  092526 MF,CC  First version (issue #447).
 */

/* Using bbw_ALT1 for TA_ALT={BATCH,JAVA} */

   /**
    * Number of leading input bars {@link Core#bbw} consumes before it can
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
   public int bbwLookback( int optInTimePeriod, double optInNbDevUp, double optInNbDevDn, MAType optInMAType )
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
   RetCode bbwImpl( int startIdx,
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
      int offset = 0;
      double middle = 0;
      double deviation = 0;
      double tempReal = 0;
      double upper = 0;
      double lower = 0;
      double[] tempBuffer;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OUT_OF_RANGE_START_INDEX ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
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
         int _zero;
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
            _zero = 0;
            do {
               maTotal += inReal[_i];
               _tempReal = inReal[_i] - shift;
               varTotal1 += _tempReal;
               _tempReal *= _tempReal;
               varTotal2 += _tempReal;
               _peakTotal2 = (varTotal2 > _peakTotal2) ? varTotal2 : _peakTotal2;
               meanValue1 = varTotal1 * _invPeriod;
               variance = varTotal2 * _invPeriod - meanValue1 * meanValue1;
               _mid[_t] = maTotal / optInTimePeriod;
               if( _mid[_t] == 0.0 ) {
                  _zero = 1;
               }
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
                  upper = middle + tempReal;
                  lower = middle - tempReal;
                  _var[_k] = (upper - lower) / middle;
               }
            } else {
               for( _k = 0; _k < _t; _k += 1 ) {
                  middle = _mid[_k];
                  deviation = Math.sqrt(_var[_k]);
                  upper = Math.fma(deviation, optInNbDevUp, middle);
                  lower = middle - deviation * optInNbDevDn;
                  _var[_k] = (upper - lower) / middle;
               }
            }
            if( _zero != 0 ) {
               for( _k = 0; _k < _t; _k += 1 ) {
                  if( _mid[_k] == 0.0 ) {
                     _var[_k] = 0.0;
                  }
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
      if( bbwLookback(optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType) > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.SUCCESS ;
      }
      tempBuffer = new double[(int)((endIdx - startIdx + 1) * 1)];
      OutRange _xr0 = ma(startIdx, endIdx, inReal, optInTimePeriod, optInMAType, tempBuffer);
      maBegIdx.value = _xr0.begIdx();
      maNbElement.value = _xr0.count();
      retCode = RetCode.SUCCESS;
      OutRange _xr1 = var(maBegIdx.value, endIdx, inReal, optInTimePeriod, 1.0, outReal);
      outBegIdx.value = _xr1.begIdx();
      outNBElement.value = _xr1.count();
      retCode = RetCode.SUCCESS;
      offset = maNbElement.value - outNBElement.value;
      if( offset != 0 ) {
         System.arraycopy(tempBuffer, offset, tempBuffer, 0, outNBElement.value * 1);
      }
      if( optInNbDevUp == optInNbDevDn ) {
         for( i = 0; i < (int)outNBElement.value; i += 1 ) {
            middle = tempBuffer[i];
            tempReal = Math.sqrt(outReal[i]) * optInNbDevUp;
            upper = middle + tempReal;
            lower = middle - tempReal;
            outReal[i] = (upper - lower) / middle;
         }
      } else {
         for( i = 0; i < (int)outNBElement.value; i += 1 ) {
            middle = tempBuffer[i];
            deviation = Math.sqrt(outReal[i]);
            upper = Math.fma(deviation, optInNbDevUp, middle);
            lower = middle - deviation * optInNbDevDn;
            outReal[i] = (upper - lower) / middle;
         }
      }
      for( i = 0; i < (int)outNBElement.value; i += 1 ) {
         if( tempBuffer[i] == 0.0 ) {
            outReal[i] = 0.0;
         }
      }
      return RetCode.SUCCESS ;
   }
   RetCode bbwImpl( int startIdx,
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
      int offset = 0;
      double middle = 0;
      double deviation = 0;
      double tempReal = 0;
      double upper = 0;
      double lower = 0;
      double[] tempBuffer;
      if( (startIdx < 0) || (startIdx > MAX_INDEX) ) {
         return RetCode.OUT_OF_RANGE_START_INDEX ;
      }
      if( (endIdx < 0) || (endIdx > MAX_INDEX) || (endIdx < startIdx)) {
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
         int _zero;
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
            _zero = 0;
            do {
               maTotal += (double)inReal[_i];
               _tempReal = (double)inReal[_i] - shift;
               varTotal1 += _tempReal;
               _tempReal *= _tempReal;
               varTotal2 += _tempReal;
               _peakTotal2 = (varTotal2 > _peakTotal2) ? varTotal2 : _peakTotal2;
               meanValue1 = varTotal1 * _invPeriod;
               variance = varTotal2 * _invPeriod - meanValue1 * meanValue1;
               _mid[_t] = maTotal / optInTimePeriod;
               if( _mid[_t] == 0.0 ) {
                  _zero = 1;
               }
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
                  upper = middle + tempReal;
                  lower = middle - tempReal;
                  _var[_k] = (upper - lower) / middle;
               }
            } else {
               for( _k = 0; _k < _t; _k += 1 ) {
                  middle = _mid[_k];
                  deviation = Math.sqrt(_var[_k]);
                  upper = Math.fma(deviation, optInNbDevUp, middle);
                  lower = middle - deviation * optInNbDevDn;
                  _var[_k] = (upper - lower) / middle;
               }
            }
            if( _zero != 0 ) {
               for( _k = 0; _k < _t; _k += 1 ) {
                  if( _mid[_k] == 0.0 ) {
                     _var[_k] = 0.0;
                  }
               }
            }
            System.arraycopy(_var, 0, outReal, _outIdx, _t * 1);
            _outIdx += _t;
         } while( _i <= endIdx );
         outNBElement.value = _outIdx;
         outBegIdx.value = startIdx;
         return RetCode.SUCCESS ;
      }
      if( bbwLookback(optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType) > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.SUCCESS ;
      }
      tempBuffer = new double[(int)((endIdx - startIdx + 1) * 1)];
      OutRange _xr0 = ma(startIdx, endIdx, inReal, optInTimePeriod, optInMAType, tempBuffer);
      maBegIdx.value = _xr0.begIdx();
      maNbElement.value = _xr0.count();
      retCode = RetCode.SUCCESS;
      OutRange _xr1 = var(maBegIdx.value, endIdx, inReal, optInTimePeriod, 1.0, outReal);
      outBegIdx.value = _xr1.begIdx();
      outNBElement.value = _xr1.count();
      retCode = RetCode.SUCCESS;
      offset = maNbElement.value - outNBElement.value;
      if( offset != 0 ) {
         System.arraycopy(tempBuffer, offset, tempBuffer, 0, outNBElement.value * 1);
      }
      if( optInNbDevUp == optInNbDevDn ) {
         for( i = 0; i < (int)outNBElement.value; i += 1 ) {
            middle = tempBuffer[i];
            tempReal = Math.sqrt(outReal[i]) * optInNbDevUp;
            upper = middle + tempReal;
            lower = middle - tempReal;
            outReal[i] = (upper - lower) / middle;
         }
      } else {
         for( i = 0; i < (int)outNBElement.value; i += 1 ) {
            middle = tempBuffer[i];
            deviation = Math.sqrt(outReal[i]);
            upper = Math.fma(deviation, optInNbDevUp, middle);
            lower = middle - deviation * optInNbDevDn;
            outReal[i] = (upper - lower) / middle;
         }
      }
      for( i = 0; i < (int)outNBElement.value; i += 1 ) {
         if( tempBuffer[i] == 0.0 ) {
            outReal[i] = 0.0;
         }
      }
      return RetCode.SUCCESS ;
   }
   /**
    * Bollinger BandWidth: the distance between the upper and lower Bollinger
    * Bands, normalised by the middle band. Low values mark contracting
    * volatility, the setup John Bollinger calls the Squeeze; high values mark
    * expanding volatility.
    * <p>Formula and more info at <a
    * href="https://ta-lib.org/functions/bbw">ta-lib.org/functions/bbw</a>.
    * <p><b>Notes</b>
    * <ul>
    * <li>With Bollinger's settings (a simple moving average and two deviations on each side) BBW is four times the window's coefficient of variation: its standard deviation divided by its mean.</li>
    * <li>The two deviation multipliers enter only through their sum.</li>
    * <li>The result is a ratio; multiply by 100 to read it as a percentage of the middle band.</li>
    * <li>Any {@code optInMAType} other than SMA is a TA-Lib generalisation, as it is for BBANDS: the deviation stays the population standard deviation about the simple mean.</li>
    * <li>Wherever the middle band is not 0, BBW is bit for bit {@code (upper - lower) / middle} computed from BBANDS' own outputs.</li>
    * </ul>
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#bbwLookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
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
    * @param outReal Width of the bands as a fraction of the middle band. Must
    *        hold at least {@code endIdx - startIdx + 1} values.
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
    * @see Core#bbands
    * @see Core#stddev
    * @see Core#natr
    */
   public OutRange bbw( int startIdx,
                        int endIdx,
                        double inReal[],
                        int optInTimePeriod,
                        double optInNbDevUp,
                        double optInNbDevDn,
                        MAType optInMAType,
                        double outReal[] )
   {
      requireIndexRange("BBW", startIdx, endIdx);
      requireArgument("BBW", "optInMAType", optInMAType);
      int guardStart = clampedStart("BBW", startIdx, bbwLookback(optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("BBW", "inReal", inReal, guardInLen);
      requireLength("BBW", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = bbwImpl(startIdx, endIdx, inReal, optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.SUCCESS ) {
         throw failure("BBW", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
   /**
    * Bollinger BandWidth: the distance between the upper and lower Bollinger
    * Bands, normalised by the middle band. Low values mark contracting
    * volatility, the setup John Bollinger calls the Squeeze; high values mark
    * expanding volatility.
    * <p>Formula and more info at <a
    * href="https://ta-lib.org/functions/bbw">ta-lib.org/functions/bbw</a>.
    * <p><b>Notes</b>
    * <ul>
    * <li>With Bollinger's settings (a simple moving average and two deviations on each side) BBW is four times the window's coefficient of variation: its standard deviation divided by its mean.</li>
    * <li>The two deviation multipliers enter only through their sum.</li>
    * <li>The result is a ratio; multiply by 100 to read it as a percentage of the middle band.</li>
    * <li>Any {@code optInMAType} other than SMA is a TA-Lib generalisation, as it is for BBANDS: the deviation stays the population standard deviation about the simple mean.</li>
    * <li>Wherever the middle band is not 0, BBW is bit for bit {@code (upper - lower) / middle} computed from BBANDS' own outputs.</li>
    * </ul>
    * <p>This is the {@code float[]} overload. The arithmetic is performed in
    * {@code double} before being written to the {@code double[]} output, so a
    * result beyond {@code float} range is still representable.
    * <p>Values are written only where the indicator is defined. The returned
    * {@link OutRange} says where they start and how many there are; nothing
    * outside that range is touched, and the library never pads with NaN. A
    * valid range shorter than {@link Core#bbwLookback} is a <b>success with no
    * values</b> ({@code count() == 0}), not an error.
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
    * @param outReal Width of the bands as a fraction of the middle band. Must
    *        hold at least {@code endIdx - startIdx + 1} values.
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
    * @see Core#bbands
    * @see Core#stddev
    * @see Core#natr
    */
   public OutRange bbw( int startIdx,
                        int endIdx,
                        float inReal[],
                        int optInTimePeriod,
                        double optInNbDevUp,
                        double optInNbDevDn,
                        MAType optInMAType,
                        double outReal[] )
   {
      requireIndexRange("BBW", startIdx, endIdx);
      requireArgument("BBW", "optInMAType", optInMAType);
      int guardStart = clampedStart("BBW", startIdx, bbwLookback(optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType));
      int guardInLen = endIdx + 1;
      int guardOutLen = guardStart > endIdx ? 0 : endIdx - guardStart + 1;
      requireLength("BBW", "inReal", inReal, guardInLen);
      requireLength("BBW", "outReal", outReal, guardOutLen);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      RetCode retCode = bbwImpl(startIdx, endIdx, inReal, optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType, outBegIdx, outNBElement, outReal);
      if( retCode != RetCode.SUCCESS ) {
         throw failure("BBW", retCode);
      }
      return new OutRange(outBegIdx.value, outNBElement.value);
   }
/**** Streaming API *****/

   /**
    * A live BBW stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#bbw} over the same series.
    * Open with {@link Core#bbwOpen}; there is no close — the handle is
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
   public static final class BbwStream {
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

      private BbwStream( Core core ) { this.core = core; }

      /**
       * The bars this stream has an output for, in the input series'
       * coordinates: {@code [begIdx, begIdx + count)}.
       * <p>It is what {@link Core#bbw} reports over the same bars: the
       * opener sets it to {@code (lookback, historyLen - lookback)}, every
       * accepted {@code update} adds one to the count — a rejected one
       * changes nothing, and neither does {@code peek} — and
       * {@code clone()} carries it verbatim. A plain
       * {@code open} hands back only the last value, a subset of this range,
       * because the caller chose not to take the fill.
       * <p>The last bar it can reach is {@link Core#MAX_INDEX}; past that
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
       * has reached bar {@link Core#MAX_INDEX}, the last one the batch tier
       * can address and the last this handle will count. {@code update}
       * throws the same there.
       */
      public void advance() {
         if( this.outRangeBegIdx + this.outRangeCount > MAX_INDEX )
            throw failure("BBW advance", RetCode.OUT_OF_RANGE_END_INDEX);
         this.outRangeCount++;
      }

      private BbwStream( BbwStream other ) {
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
       * has reached bar {@link Core#MAX_INDEX}, which no re-feed clears: the
       * handle has run out of index domain and only a shorter history can
       * start a new one.
       */
      public double update( double inReal ) {
         if( this.outRangeBegIdx + this.outRangeCount > MAX_INDEX )
            throw failure("BBW update", RetCode.OUT_OF_RANGE_END_INDEX);
         if( !Double.isFinite(inReal) )
            throw new TALibArgumentException("BBW update: BAD_PARAM", RetCode.BAD_PARAM);
         core.bbwStepImpl(this, inReal);
         this.outRangeCount++;
         return this.cur_outReal;
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return — the same
       * transition, with every store it would make carried in a local instead.
       * Never writes this handle, so peeks may
       * run concurrently with each other, and its cost does not grow with the
       * period.
       * <p>It counts no bar, so it keeps answering past the
       * {@link Core#MAX_INDEX} ceiling {@code update} stops at.
       */
      public double peek( double inReal ) {
         if( !Double.isFinite(inReal) )
            throw new TALibArgumentException("BBW peek: BAD_PARAM", RetCode.BAD_PARAM);
         BbwStream sp = this;
         double deviation = 0.0;
         double lower = 0.0;
         double middle = 0.0;
         double tempReal = 0.0;
         double upper = 0.0;
         double cur_tempBuffer = 0.0;
         double cur_outReal = 0.0;
         /* Pipeline the new bar through the sub-streams (batch tail order). */
         cur_tempBuffer = sp.sub0.peek(inReal);
         cur_outReal = sp.sub1.peek(inReal);
         /* Combine map (batch tail, per bar). */
         if( sp.optInNbDevUp == sp.optInNbDevDn ) {
            middle = cur_tempBuffer;
            tempReal = Math.sqrt(cur_outReal) * sp.optInNbDevUp;
            upper = middle + tempReal;
            lower = middle - tempReal;
            cur_outReal = (upper - lower) / middle;
            if( middle == 0.0 ) {
               cur_outReal = 0.0;
            }
         } else {
            middle = cur_tempBuffer;
            deviation = Math.sqrt(cur_outReal);
            upper = Math.fma(deviation, sp.optInNbDevUp, middle);
            lower = middle - deviation * sp.optInNbDevDn;
            cur_outReal = (upper - lower) / middle;
            if( middle == 0.0 ) {
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
      public BbwStream clone() {
         return new BbwStream(this);
      }
   }
   private void bbwStepImpl( BbwStream sp, double inReal )
   {
      double deviation = 0.0;
      double lower = 0.0;
      double middle = 0.0;
      double tempReal = 0.0;
      double upper = 0.0;
      double cur_tempBuffer = 0.0;
      double cur_outReal = 0.0;
      /* Pipeline the new bar through the sub-streams (batch tail order). */
      cur_tempBuffer = sp.sub0.update(inReal);
      cur_outReal = sp.sub1.update(inReal);
      /* Combine map (batch tail, per bar). */
      if( sp.optInNbDevUp == sp.optInNbDevDn ) {
         middle = cur_tempBuffer;
         tempReal = Math.sqrt(cur_outReal) * sp.optInNbDevUp;
         upper = middle + tempReal;
         lower = middle - tempReal;
         cur_outReal = (upper - lower) / middle;
         if( middle == 0.0 ) {
            cur_outReal = 0.0;
         }
      } else {
         middle = cur_tempBuffer;
         deviation = Math.sqrt(cur_outReal);
         upper = Math.fma(deviation, sp.optInNbDevUp, middle);
         lower = middle - deviation * sp.optInNbDevDn;
         cur_outReal = (upper - lower) / middle;
         if( middle == 0.0 ) {
            cur_outReal = 0.0;
         }
      }
      sp.cur_outReal = cur_outReal;
   }
   private RetCode bbwOpenImpl( BbwStream sp, double inReal[], int startIdx, int optInTimePeriod, double optInNbDevUp, double optInNbDevDn, MAType optInMAType, MInteger outBegIdx, MInteger outNBElement, double outReal[], int outStride )
   {
      RetCode retCode;
      int i = 0;
      MInteger maBegIdx = new MInteger();
      MInteger maNbElement = new MInteger();
      int offset = 0;
      double middle = 0;
      double deviation = 0;
      double tempReal = 0;
      double upper = 0;
      double lower = 0;
      double[] tempBuffer;
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.OUT_OF_RANGE_START_INDEX;
      }
      if( historyLen > MAX_INDEX + 1 ) {
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
      if( historyLen < bbwLookback(optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType) + 1 ) {
         return RetCode.INSUFFICIENT_HISTORY;
      }
      double[] sc_outReal = outStride == 1 ? outReal : new double[historyLen];
      if( bbwLookback(optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType) > endIdx ) {
         outBegIdx.value = 0;
         outNBElement.value = 0;
         return RetCode.INSUFFICIENT_HISTORY ;
      }
      tempBuffer = new double[(int)((endIdx - startIdx + 1) * 1)];
      /* Before the variance: it may be written over inReal. */
      /* Sub-stream 0: ma over `inReal`, warmed from bar 0 up to the
       * sub-call's own startIdx (the seeding point). */
      MaStream sub0 = maOpenAndFillInternal(inReal, startIdx, optInTimePeriod, optInMAType, maBegIdx, maNbElement, tempBuffer);
      retCode = RetCode.SUCCESS;
      /* From the moving average's begIdx, as TA_BBANDS enters its deviation:
       * the variance's shift and reseed schedule are anchored on its start.
       */
      /* Sub-stream 1: var over `inReal`, warmed from bar 0 up to the
       * sub-call's own startIdx (the seeding point). */
      VarStream sub1 = varOpenAndFillInternal(inReal, maBegIdx.value, optInTimePeriod, 1.0, outBegIdx, outNBElement, sc_outReal);
      retCode = RetCode.SUCCESS;
      offset = maNbElement.value - outNBElement.value;
      if( optInNbDevUp == optInNbDevDn ) {
         for( i = 0; i < (int)outNBElement.value; i += 1 ) {
            middle = tempBuffer[i + offset];
            tempReal = Math.sqrt(sc_outReal[i]) * optInNbDevUp;
            upper = middle + tempReal;
            lower = middle - tempReal;
            sc_outReal[i] = (upper - lower) / middle;
            if( middle == 0.0 ) {
               sc_outReal[i] = 0.0;
            }
         }
      } else {
         for( i = 0; i < (int)outNBElement.value; i += 1 ) {
            middle = tempBuffer[i + offset];
            deviation = Math.sqrt(sc_outReal[i]);
            upper = Math.fma(deviation, optInNbDevUp, middle);
            lower = middle - deviation * optInNbDevDn;
            sc_outReal[i] = (upper - lower) / middle;
            if( middle == 0.0 ) {
               sc_outReal[i] = 0.0;
            }
         }
      }
      /* Capture the live producer state + sub handles. */
      if( outNBElement.value < 1 ) {
         return RetCode.INSUFFICIENT_HISTORY;
      }
      sp.optInTimePeriod = optInTimePeriod;
      sp.optInNbDevUp = optInNbDevUp;
      sp.optInNbDevDn = optInNbDevDn;
      sp.optInMAType = optInMAType;
      sp.sub0 = sub0;
      sp.sub1 = sub1;
      sp.cur_outReal = sc_outReal[outNBElement.value - 1];
      return RetCode.SUCCESS;
   }
   /* bbwOpenAndFill anchored at startIdx — the composed-open fusion seam. */
   BbwStream bbwOpenAndFillInternal( double inReal[], int startIdx, int optInTimePeriod, double optInNbDevUp, double optInNbDevDn, MAType optInMAType, MInteger outBegIdx, MInteger outNBElement, double outReal[] )
   {
      BbwStream sp = new BbwStream(this);
      RetCode retCode = bbwOpenImpl(sp, inReal, startIdx, optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType, outBegIdx, outNBElement, outReal, 1);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.SUCCESS ) {
         return sp;
      }
      if( retCode == RetCode.INSUFFICIENT_HISTORY ) {
         throw new InsufficientHistoryException("BBW openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.INTERNAL_ERROR ) {
         throw new TALibStateException("BBW openAndFill: internal error", retCode);
      }
      throw new TALibArgumentException("BBW openAndFill: " + retCode, retCode);
   }
   /* Internal startIdx-anchored open behind bbwOpen (composition seam). */
   BbwStream bbwOpenInternal( double inReal[], int startIdx, int optInTimePeriod, double optInNbDevUp, double optInNbDevDn, MAType optInMAType )
   {
      BbwStream sp = new BbwStream(this);
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      double[] sink_outReal = new double[1];
      RetCode retCode = bbwOpenImpl(sp, inReal, startIdx, optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType, outBegIdx, outNBElement, sink_outReal, 0);
      sp.outRangeBegIdx = outBegIdx.value;
      sp.outRangeCount = outNBElement.value;
      if( retCode == RetCode.SUCCESS ) {
         return sp;
      }
      if( retCode == RetCode.INSUFFICIENT_HISTORY ) {
         throw new InsufficientHistoryException("BBW open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.INTERNAL_ERROR ) {
         throw new TALibStateException("BBW open: internal error", retCode);
      }
      throw new TALibArgumentException("BBW open: " + retCode, retCode);
   }
   /**
    * Open a live BBW stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#bbw} at that bar.
    * <p>The history must hold at least {@code bbwLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@link Integer#MIN_VALUE}, {@link Core#REAL_DEFAULT} and
    * {@link MAType#DEFAULT} select a parameter's documented default, as in
    * the batch API). An EMPTY history throws
    * {@link IndexOutOfBoundsException} — its implied {@code startIdx} of 0
    * names no bar — and a null argument {@link IllegalArgumentException},
    * both ahead of everything above.
    */
   public BbwStream bbwOpen( double inReal[], int optInTimePeriod, double optInNbDevUp, double optInNbDevDn, MAType optInMAType )
   {
      requireArgument("BBW open", "inReal", inReal);
      requireHistory("BBW open", inReal.length);
      requireArgument("BBW open", "optInMAType", optInMAType);
      return bbwOpenInternal(inReal, 0, optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType);
   }
   /**
    * {@link Core#bbwOpen} that also fills the output array(s) bit-identically
    * to {@link Core#bbw} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values — both checked before anything is
    * written, so an undersized array is an {@link IllegalArgumentException}
    * naming it rather than a fault from inside the fill.
    * <p>The range written is on the returned handle:
    * {@link BbwStream#outRange()}.
    */
   public BbwStream bbwOpenAndFill( double inReal[], int optInTimePeriod, double optInNbDevUp, double optInNbDevDn, MAType optInMAType, double outReal[] )
   {
      requireArgument("BBW openAndFill", "inReal", inReal);
      requireHistory("BBW openAndFill", inReal.length);
      requireArgument("BBW openAndFill", "optInMAType", optInMAType);
      int guardOutLen = openFillCount("BBW openAndFill", inReal.length, bbwLookback(optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType));
      requireLength("BBW openAndFill", "outReal", outReal, guardOutLen);
      if( (Object)outReal == (Object)inReal ) {
         throw new TALibArgumentException("BBW openAndFill: " + RetCode.BAD_PARAM, RetCode.BAD_PARAM);
      }
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      return bbwOpenAndFillInternal(inReal, 0, optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType, outBegIdx, outNBElement, outReal);
   }
