/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  JV       Jesus Viver <324122@cienz.unizar.es>
 *  CC       Claude Code (AI assistant)
 *
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  112400 MF     Template creation.
 *  010503 MF     Fix to always use SMA for the STDDEV (Thanks to JV).
 *  052603 MF     Adapt code to compile with .NET Managed C++
 *  070626 MF,CC  Fix #99: realign the middle band when the standard
 *                deviation clamps to a later begIdx than the
 *                (period-independent) MAMA lookback, for
 *                optInTimePeriod >= 34.
 *  071126 MF,CC  Split into an SMA fast path (reuses the moving average as the
 *                mean) and a general MA + STDDEV path, so BBANDS streams as a
 *                composition of the TA_MA and TA_STDDEV streams. Bit-identical.
 *  071626 MF,CC  #117 speed optimization: fuse the SMA fast path's moving
 *                average and standard deviation into a single pass. Bit-identical.
 *  071726 MF,CC  #118 SMA-path deviation now uses the cancellation-free variance
 *                (var.c); two recurrences in one pass. Bit-identical.
 */

   public int bbandsLookback( int optInTimePeriod, double optInNbDevUp, double optInNbDevDn, MAType optInMAType )
   {
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 5;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return -1;
      }
      if( optInNbDevUp == -4e37 ) {
         optInNbDevUp = 2e0;
      }
      if( optInNbDevDn == -4e37 ) {
         optInNbDevDn = 2e0;
      }
      /* The lookback is driven by the middle band moving average. It also governs
       * how the caller sizes the output buffers, which must hold the full moving
       * average that ma() writes below - so it must not exceed the MA lookback,
       * even when the standard deviation (lookback optInTimePeriod-1) clamps the
       * first output to a later bar (outBegIdx > lookback for TA_MAType_MAMA with
       * a large period). See the realignment in bbands() for that case.
       */
      return movingAverageLookback(optInTimePeriod, optInMAType) ;

   }
   public RetCode bbands( int startIdx,
                          int endIdx,
                          double inReal[],
                          int optInTimePeriod,
                          double optInNbDevUp,
                          double optInNbDevDn,
                          MAType optInMAType,
                          MInteger outBegIdx,
                          MInteger outNBElement,
                          double outRealUpperBand[],
                          double outRealMiddleBand[],
                          double outRealLowerBand[] )
   {
      RetCode retCode;
      int i = 0;
      int maBegIdx = 0;
      int shiftIdx = 0;
      double tempReal = 0;
      double tempReal2 = 0;
      double[] tempBuffer1;
      double[] tempBuffer2;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 5;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInNbDevUp == -4e37 ) {
         optInNbDevUp = 2e0;
      }
      if( optInNbDevDn == -4e37 ) {
         optInNbDevDn = 2e0;
      }
      if( outRealUpperBand == outRealMiddleBand || outRealUpperBand == outRealLowerBand || outRealMiddleBand == outRealLowerBand ) {
         return RetCode.BadParam ;
      }
      if( optInMAType == MAType.Sma ) {
         /* SMA fast path: the middle band (SMA) and the standard deviation share one
          * pass over the window below. Bit-identical to the general MA + STDDEV path
          * (which the stream composes for every MA type).
          *
          * Identify TWO temporary buffers among the outputs so the calculation
          * needs no memory allocation; whenever possible make tempBuffer1 be the
          * middle band output, saving one copy operation.
          */
         if( inReal == outRealUpperBand ) {
            tempBuffer1 = outRealMiddleBand;
            tempBuffer2 = outRealLowerBand;
         } else if( inReal == outRealLowerBand ) {
            tempBuffer1 = outRealMiddleBand;
            tempBuffer2 = outRealUpperBand;
         } else if( inReal == outRealMiddleBand ) {
            tempBuffer1 = outRealLowerBand;
            tempBuffer2 = outRealUpperBand;
         } else {
            tempBuffer1 = outRealMiddleBand;
            tempBuffer2 = outRealUpperBand;
         }
         /* Check that the caller is not doing tricky things.
          * (like using the input buffer in two output!)
          */
         if( tempBuffer1 == inReal || tempBuffer2 == inReal ) {
            return RetCode.BadParam ;
         }
         /* One pass with two independent recurrences: the SMA running sum (maTotal,
          * mean -> tempBuffer1, bit-identical to TA_MA(SMA)) and the shifted-data
          * variance (-> tempBuffer2, bit-identical to TA_STDDEV/TA_VAR - see var.c).
          * The variance carries its own shift and reseed; the SMA sum is untouched by
          * it. tempBuffer1/2 never alias inReal (checked above).
          */
         double maTotal;
         double shift;
         double varTotal1;
         double varTotal2;
         double meanValue1;
         double variance;
         double _invPeriod;
         double _tempReal;
         int _i;
         int _j;
         int _outIdx;
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
            return RetCode.Success ;
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
         do {
            maTotal += inReal[_i];
            _tempReal = inReal[_i] - shift;
            varTotal1 += _tempReal;
            _tempReal *= _tempReal;
            varTotal2 += _tempReal;
            meanValue1 = varTotal1 * _invPeriod;
            variance = varTotal2 * _invPeriod - meanValue1 * meanValue1;
            tempBuffer1[_outIdx] = maTotal / optInTimePeriod;
            maTotal -= inReal[_trailingIdx];
            _tempReal = inReal[_trailingIdx] - shift;
            varTotal1 -= _tempReal;
            _tempReal *= _tempReal;
            varTotal2 -= _tempReal;
            _trailingIdx += 1;
            _barsSinceReseed -= 1;
            if( variance < 0.000001 * (varTotal2 * _invPeriod) || _tempReal > 1000000.0 * varTotal2 || _barsSinceReseed <= 0 ) {
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
               _tempReal = inReal[_windowStart] - shift;
               varTotal1 -= _tempReal;
               _tempReal *= _tempReal;
               varTotal2 -= _tempReal;
            }
            if( !(variance < 0.00000000000001) ) {
               tempBuffer2[_outIdx] = Math.sqrt(variance);
            } else {
               tempBuffer2[_outIdx] = 0.0;
            }
            _outIdx += 1;
            _i += 1;
         } while( _i <= endIdx );
         outNBElement.value = _outIdx;
         outBegIdx.value = startIdx;
         /* Copy the MA calculation into the middle band ouput, unless
          * the calculation was done into it already!
          */
         if( tempBuffer1 != outRealMiddleBand ) {
            System.arraycopy(tempBuffer1, 0, outRealMiddleBand, 0, outNBElement.value * 1);
         }
         /* Now do a tight loop to calculate the upper/lower band at the same time. */
         if( optInNbDevUp == optInNbDevDn ) {
            for( i = 0; i < (int)outNBElement.value; i += 1 ) {
               tempReal = tempBuffer2[i] * optInNbDevUp;
               tempReal2 = outRealMiddleBand[i];
               outRealUpperBand[i] = tempReal2 + tempReal;
               outRealLowerBand[i] = tempReal2 - tempReal;
            }
         } else {
            for( i = 0; i < (int)outNBElement.value; i += 1 ) {
               tempReal = tempBuffer2[i];
               tempReal2 = outRealMiddleBand[i];
               outRealUpperBand[i] = Math.fma(tempReal, optInNbDevUp, tempReal2);
               outRealLowerBand[i] = tempReal2 - tempReal * optInNbDevDn;
            }
         }
         return RetCode.Success ;
      }
      /* General path (every MA type other than SMA): the middle band is the moving
       * average and the deviation is the standard deviation of the input, combined
       * at the same bar. Two intermediate buffers are allocated so the input may
       * safely alias an output (it is only read here).
       */
      tempBuffer1 = new double[(int)((endIdx - startIdx + 1) * 1)];
      tempBuffer2 = new double[(int)((endIdx - startIdx + 1) * 1)];
      /* Calculate the middle band moving average. */
      retCode = movingAverageUnguarded(startIdx, endIdx, inReal, optInTimePeriod, optInMAType, outBegIdx, outNBElement, tempBuffer1);
      if( retCode != RetCode.Success || (int)outNBElement.value == 0 ) {
         outNBElement.value = 0;
         return retCode ;
      }
      /* Remember where the moving average begins, to realign it below. */
      maBegIdx = (int)outBegIdx.value;
      /* Calculate the Standard Deviation into tempBuffer2. */
      retCode = stdDevUnguarded((int)outBegIdx.value, endIdx, inReal, optInTimePeriod, 1.0, outBegIdx, outNBElement, tempBuffer2);
      if( retCode != RetCode.Success ) {
         outNBElement.value = 0;
         return retCode ;
      }
      /* When the standard deviation (lookback optInTimePeriod-1) clamps to a later
       * begIdx than the moving average did - as with TA_MAType_MAMA (constant
       * lookback 32) and optInTimePeriod >= 34 - the MA in tempBuffer1 still starts
       * at the earlier maBegIdx. Copy it forward from that shift into the middle
       * band so each band value pairs the moving average and standard deviation of
       * the same bar. The guarded subtraction keeps shiftIdx non-negative even when
       * the standard deviation produced no output (an empty range leaves *outBegIdx
       * at 0), which the unconditional copy below then handles as a zero-length move.
       */
      if( (int)outBegIdx.value > maBegIdx ) {
         shiftIdx = (int)outBegIdx.value - maBegIdx;
      } else {
         shiftIdx = 0;
      }
      System.arraycopy(tempBuffer1, shiftIdx, outRealMiddleBand, 0, outNBElement.value * 1);
      /* Now do a tight loop to calculate the upper/lower band at the same time. */
      if( optInNbDevUp == optInNbDevDn ) {
         for( i = 0; i < (int)outNBElement.value; i += 1 ) {
            tempReal = tempBuffer2[i] * optInNbDevUp;
            tempReal2 = outRealMiddleBand[i];
            outRealUpperBand[i] = tempReal2 + tempReal;
            outRealLowerBand[i] = tempReal2 - tempReal;
         }
      } else {
         for( i = 0; i < (int)outNBElement.value; i += 1 ) {
            tempReal2 = outRealMiddleBand[i];
            outRealUpperBand[i] = Math.fma(tempBuffer2[i], optInNbDevUp, tempReal2);
            outRealLowerBand[i] = tempReal2 - tempBuffer2[i] * optInNbDevDn;
         }
      }
      return RetCode.Success ;
   }
   public RetCode bbandsUnguarded( int startIdx,
                                   int endIdx,
                                   double inReal[],
                                   int optInTimePeriod,
                                   double optInNbDevUp,
                                   double optInNbDevDn,
                                   MAType optInMAType,
                                   MInteger outBegIdx,
                                   MInteger outNBElement,
                                   double outRealUpperBand[],
                                   double outRealMiddleBand[],
                                   double outRealLowerBand[] )
   {
      RetCode retCode;
      int i = 0;
      int maBegIdx = 0;
      int shiftIdx = 0;
      double tempReal = 0;
      double tempReal2 = 0;
      double[] tempBuffer1;
      double[] tempBuffer2;
      if( optInMAType == MAType.Sma ) {
         if( inReal == outRealUpperBand ) {
            tempBuffer1 = outRealMiddleBand;
            tempBuffer2 = outRealLowerBand;
         } else if( inReal == outRealLowerBand ) {
            tempBuffer1 = outRealMiddleBand;
            tempBuffer2 = outRealUpperBand;
         } else if( inReal == outRealMiddleBand ) {
            tempBuffer1 = outRealLowerBand;
            tempBuffer2 = outRealUpperBand;
         } else {
            tempBuffer1 = outRealMiddleBand;
            tempBuffer2 = outRealUpperBand;
         }
         if( tempBuffer1 == inReal || tempBuffer2 == inReal ) {
            return RetCode.BadParam ;
         }
         double maTotal;
         double shift;
         double varTotal1;
         double varTotal2;
         double meanValue1;
         double variance;
         double _invPeriod;
         double _tempReal;
         int _i;
         int _j;
         int _outIdx;
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
            return RetCode.Success ;
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
         do {
            maTotal += inReal[_i];
            _tempReal = inReal[_i] - shift;
            varTotal1 += _tempReal;
            _tempReal *= _tempReal;
            varTotal2 += _tempReal;
            meanValue1 = varTotal1 * _invPeriod;
            variance = varTotal2 * _invPeriod - meanValue1 * meanValue1;
            tempBuffer1[_outIdx] = maTotal / optInTimePeriod;
            maTotal -= inReal[_trailingIdx];
            _tempReal = inReal[_trailingIdx] - shift;
            varTotal1 -= _tempReal;
            _tempReal *= _tempReal;
            varTotal2 -= _tempReal;
            _trailingIdx += 1;
            _barsSinceReseed -= 1;
            if( variance < 0.000001 * (varTotal2 * _invPeriod) || _tempReal > 1000000.0 * varTotal2 || _barsSinceReseed <= 0 ) {
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
               _tempReal = inReal[_windowStart] - shift;
               varTotal1 -= _tempReal;
               _tempReal *= _tempReal;
               varTotal2 -= _tempReal;
            }
            if( !(variance < 0.00000000000001) ) {
               tempBuffer2[_outIdx] = Math.sqrt(variance);
            } else {
               tempBuffer2[_outIdx] = 0.0;
            }
            _outIdx += 1;
            _i += 1;
         } while( _i <= endIdx );
         outNBElement.value = _outIdx;
         outBegIdx.value = startIdx;
         if( tempBuffer1 != outRealMiddleBand ) {
            System.arraycopy(tempBuffer1, 0, outRealMiddleBand, 0, outNBElement.value * 1);
         }
         if( optInNbDevUp == optInNbDevDn ) {
            for( i = 0; i < (int)outNBElement.value; i += 1 ) {
               tempReal = tempBuffer2[i] * optInNbDevUp;
               tempReal2 = outRealMiddleBand[i];
               outRealUpperBand[i] = tempReal2 + tempReal;
               outRealLowerBand[i] = tempReal2 - tempReal;
            }
         } else {
            for( i = 0; i < (int)outNBElement.value; i += 1 ) {
               tempReal = tempBuffer2[i];
               tempReal2 = outRealMiddleBand[i];
               outRealUpperBand[i] = Math.fma(tempReal, optInNbDevUp, tempReal2);
               outRealLowerBand[i] = tempReal2 - tempReal * optInNbDevDn;
            }
         }
         return RetCode.Success ;
      }
      tempBuffer1 = new double[(int)((endIdx - startIdx + 1) * 1)];
      tempBuffer2 = new double[(int)((endIdx - startIdx + 1) * 1)];
      retCode = movingAverageUnguarded(startIdx, endIdx, inReal, optInTimePeriod, optInMAType, outBegIdx, outNBElement, tempBuffer1);
      if( retCode != RetCode.Success || (int)outNBElement.value == 0 ) {
         outNBElement.value = 0;
         return retCode ;
      }
      maBegIdx = (int)outBegIdx.value;
      retCode = stdDevUnguarded((int)outBegIdx.value, endIdx, inReal, optInTimePeriod, 1.0, outBegIdx, outNBElement, tempBuffer2);
      if( retCode != RetCode.Success ) {
         outNBElement.value = 0;
         return retCode ;
      }
      if( (int)outBegIdx.value > maBegIdx ) {
         shiftIdx = (int)outBegIdx.value - maBegIdx;
      } else {
         shiftIdx = 0;
      }
      System.arraycopy(tempBuffer1, shiftIdx, outRealMiddleBand, 0, outNBElement.value * 1);
      if( optInNbDevUp == optInNbDevDn ) {
         for( i = 0; i < (int)outNBElement.value; i += 1 ) {
            tempReal = tempBuffer2[i] * optInNbDevUp;
            tempReal2 = outRealMiddleBand[i];
            outRealUpperBand[i] = tempReal2 + tempReal;
            outRealLowerBand[i] = tempReal2 - tempReal;
         }
      } else {
         for( i = 0; i < (int)outNBElement.value; i += 1 ) {
            tempReal2 = outRealMiddleBand[i];
            outRealUpperBand[i] = Math.fma(tempBuffer2[i], optInNbDevUp, tempReal2);
            outRealLowerBand[i] = tempReal2 - tempBuffer2[i] * optInNbDevDn;
         }
      }
      return RetCode.Success ;
   }
   public RetCode bbands( int startIdx,
                          int endIdx,
                          float inReal[],
                          int optInTimePeriod,
                          double optInNbDevUp,
                          double optInNbDevDn,
                          MAType optInMAType,
                          MInteger outBegIdx,
                          MInteger outNBElement,
                          double outRealUpperBand[],
                          double outRealMiddleBand[],
                          double outRealLowerBand[] )
   {
      RetCode retCode;
      int i = 0;
      int maBegIdx = 0;
      int shiftIdx = 0;
      double tempReal = 0;
      double tempReal2 = 0;
      double[] tempBuffer1;
      double[] tempBuffer2;
      if( startIdx < 0 ) {
         return RetCode.OutOfRangeStartIndex ;
      }
      if( (endIdx < 0) || (endIdx < startIdx)) {
         return RetCode.OutOfRangeEndIndex ;
      }
      if( optInTimePeriod == Integer.MIN_VALUE ) {
         optInTimePeriod = 5;
      } else if( optInTimePeriod < 2 || optInTimePeriod > 100000 ) {
         return RetCode.BadParam;
      }
      if( optInNbDevUp == -4e37 ) {
         optInNbDevUp = 2e0;
      }
      if( optInNbDevDn == -4e37 ) {
         optInNbDevDn = 2e0;
      }
      if( outRealUpperBand == outRealMiddleBand || outRealUpperBand == outRealLowerBand || outRealMiddleBand == outRealLowerBand ) {
         return RetCode.BadParam ;
      }
      if( optInMAType == MAType.Sma ) {
         if( false ) {
            tempBuffer1 = outRealMiddleBand;
            tempBuffer2 = outRealLowerBand;
         } else if( false ) {
            tempBuffer1 = outRealMiddleBand;
            tempBuffer2 = outRealUpperBand;
         } else if( false ) {
            tempBuffer1 = outRealLowerBand;
            tempBuffer2 = outRealUpperBand;
         } else {
            tempBuffer1 = outRealMiddleBand;
            tempBuffer2 = outRealUpperBand;
         }
         if( false || false ) {
            return RetCode.BadParam ;
         }
         double maTotal;
         double shift;
         double varTotal1;
         double varTotal2;
         double meanValue1;
         double variance;
         double _invPeriod;
         double _tempReal;
         int _i;
         int _j;
         int _outIdx;
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
            return RetCode.Success ;
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
         do {
            maTotal += (double)inReal[_i];
            _tempReal = (double)inReal[_i] - shift;
            varTotal1 += _tempReal;
            _tempReal *= _tempReal;
            varTotal2 += _tempReal;
            meanValue1 = varTotal1 * _invPeriod;
            variance = varTotal2 * _invPeriod - meanValue1 * meanValue1;
            tempBuffer1[_outIdx] = maTotal / optInTimePeriod;
            maTotal -= (double)inReal[_trailingIdx];
            _tempReal = (double)inReal[_trailingIdx] - shift;
            varTotal1 -= _tempReal;
            _tempReal *= _tempReal;
            varTotal2 -= _tempReal;
            _trailingIdx += 1;
            _barsSinceReseed -= 1;
            if( variance < 0.000001 * (varTotal2 * _invPeriod) || _tempReal > 1000000.0 * varTotal2 || _barsSinceReseed <= 0 ) {
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
               _tempReal = (double)inReal[_windowStart] - shift;
               varTotal1 -= _tempReal;
               _tempReal *= _tempReal;
               varTotal2 -= _tempReal;
            }
            if( !(variance < 0.00000000000001) ) {
               tempBuffer2[_outIdx] = Math.sqrt(variance);
            } else {
               tempBuffer2[_outIdx] = 0.0;
            }
            _outIdx += 1;
            _i += 1;
         } while( _i <= endIdx );
         outNBElement.value = _outIdx;
         outBegIdx.value = startIdx;
         if( tempBuffer1 != outRealMiddleBand ) {
            System.arraycopy(tempBuffer1, 0, outRealMiddleBand, 0, outNBElement.value * 1);
         }
         if( optInNbDevUp == optInNbDevDn ) {
            for( i = 0; i < (int)outNBElement.value; i += 1 ) {
               tempReal = tempBuffer2[i] * optInNbDevUp;
               tempReal2 = outRealMiddleBand[i];
               outRealUpperBand[i] = tempReal2 + tempReal;
               outRealLowerBand[i] = tempReal2 - tempReal;
            }
         } else {
            for( i = 0; i < (int)outNBElement.value; i += 1 ) {
               tempReal = tempBuffer2[i];
               tempReal2 = outRealMiddleBand[i];
               outRealUpperBand[i] = Math.fma(tempReal, optInNbDevUp, tempReal2);
               outRealLowerBand[i] = tempReal2 - tempReal * optInNbDevDn;
            }
         }
         return RetCode.Success ;
      }
      tempBuffer1 = new double[(int)((endIdx - startIdx + 1) * 1)];
      tempBuffer2 = new double[(int)((endIdx - startIdx + 1) * 1)];
      retCode = movingAverageUnguarded(startIdx, endIdx, inReal, optInTimePeriod, optInMAType, outBegIdx, outNBElement, tempBuffer1);
      if( retCode != RetCode.Success || (int)outNBElement.value == 0 ) {
         outNBElement.value = 0;
         return retCode ;
      }
      maBegIdx = (int)outBegIdx.value;
      retCode = stdDevUnguarded((int)outBegIdx.value, endIdx, inReal, optInTimePeriod, 1.0, outBegIdx, outNBElement, tempBuffer2);
      if( retCode != RetCode.Success ) {
         outNBElement.value = 0;
         return retCode ;
      }
      if( (int)outBegIdx.value > maBegIdx ) {
         shiftIdx = (int)outBegIdx.value - maBegIdx;
      } else {
         shiftIdx = 0;
      }
      System.arraycopy(tempBuffer1, shiftIdx, outRealMiddleBand, 0, outNBElement.value * 1);
      if( optInNbDevUp == optInNbDevDn ) {
         for( i = 0; i < (int)outNBElement.value; i += 1 ) {
            tempReal = tempBuffer2[i] * optInNbDevUp;
            tempReal2 = outRealMiddleBand[i];
            outRealUpperBand[i] = tempReal2 + tempReal;
            outRealLowerBand[i] = tempReal2 - tempReal;
         }
      } else {
         for( i = 0; i < (int)outNBElement.value; i += 1 ) {
            tempReal2 = outRealMiddleBand[i];
            outRealUpperBand[i] = Math.fma(tempBuffer2[i], optInNbDevUp, tempReal2);
            outRealLowerBand[i] = tempReal2 - tempBuffer2[i] * optInNbDevDn;
         }
      }
      return RetCode.Success ;
   }
   public RetCode bbandsUnguarded( int startIdx,
                                   int endIdx,
                                   float inReal[],
                                   int optInTimePeriod,
                                   double optInNbDevUp,
                                   double optInNbDevDn,
                                   MAType optInMAType,
                                   MInteger outBegIdx,
                                   MInteger outNBElement,
                                   double outRealUpperBand[],
                                   double outRealMiddleBand[],
                                   double outRealLowerBand[] )
   {
      RetCode retCode;
      int i = 0;
      int maBegIdx = 0;
      int shiftIdx = 0;
      double tempReal = 0;
      double tempReal2 = 0;
      double[] tempBuffer1;
      double[] tempBuffer2;
      if( optInMAType == MAType.Sma ) {
         if( false ) {
            tempBuffer1 = outRealMiddleBand;
            tempBuffer2 = outRealLowerBand;
         } else if( false ) {
            tempBuffer1 = outRealMiddleBand;
            tempBuffer2 = outRealUpperBand;
         } else if( false ) {
            tempBuffer1 = outRealLowerBand;
            tempBuffer2 = outRealUpperBand;
         } else {
            tempBuffer1 = outRealMiddleBand;
            tempBuffer2 = outRealUpperBand;
         }
         if( false || false ) {
            return RetCode.BadParam ;
         }
         double maTotal;
         double shift;
         double varTotal1;
         double varTotal2;
         double meanValue1;
         double variance;
         double _invPeriod;
         double _tempReal;
         int _i;
         int _j;
         int _outIdx;
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
            return RetCode.Success ;
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
         do {
            maTotal += (double)inReal[_i];
            _tempReal = (double)inReal[_i] - shift;
            varTotal1 += _tempReal;
            _tempReal *= _tempReal;
            varTotal2 += _tempReal;
            meanValue1 = varTotal1 * _invPeriod;
            variance = varTotal2 * _invPeriod - meanValue1 * meanValue1;
            tempBuffer1[_outIdx] = maTotal / optInTimePeriod;
            maTotal -= (double)inReal[_trailingIdx];
            _tempReal = (double)inReal[_trailingIdx] - shift;
            varTotal1 -= _tempReal;
            _tempReal *= _tempReal;
            varTotal2 -= _tempReal;
            _trailingIdx += 1;
            _barsSinceReseed -= 1;
            if( variance < 0.000001 * (varTotal2 * _invPeriod) || _tempReal > 1000000.0 * varTotal2 || _barsSinceReseed <= 0 ) {
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
               _tempReal = (double)inReal[_windowStart] - shift;
               varTotal1 -= _tempReal;
               _tempReal *= _tempReal;
               varTotal2 -= _tempReal;
            }
            if( !(variance < 0.00000000000001) ) {
               tempBuffer2[_outIdx] = Math.sqrt(variance);
            } else {
               tempBuffer2[_outIdx] = 0.0;
            }
            _outIdx += 1;
            _i += 1;
         } while( _i <= endIdx );
         outNBElement.value = _outIdx;
         outBegIdx.value = startIdx;
         if( tempBuffer1 != outRealMiddleBand ) {
            System.arraycopy(tempBuffer1, 0, outRealMiddleBand, 0, outNBElement.value * 1);
         }
         if( optInNbDevUp == optInNbDevDn ) {
            for( i = 0; i < (int)outNBElement.value; i += 1 ) {
               tempReal = tempBuffer2[i] * optInNbDevUp;
               tempReal2 = outRealMiddleBand[i];
               outRealUpperBand[i] = tempReal2 + tempReal;
               outRealLowerBand[i] = tempReal2 - tempReal;
            }
         } else {
            for( i = 0; i < (int)outNBElement.value; i += 1 ) {
               tempReal = tempBuffer2[i];
               tempReal2 = outRealMiddleBand[i];
               outRealUpperBand[i] = Math.fma(tempReal, optInNbDevUp, tempReal2);
               outRealLowerBand[i] = tempReal2 - tempReal * optInNbDevDn;
            }
         }
         return RetCode.Success ;
      }
      tempBuffer1 = new double[(int)((endIdx - startIdx + 1) * 1)];
      tempBuffer2 = new double[(int)((endIdx - startIdx + 1) * 1)];
      retCode = movingAverageUnguarded(startIdx, endIdx, inReal, optInTimePeriod, optInMAType, outBegIdx, outNBElement, tempBuffer1);
      if( retCode != RetCode.Success || (int)outNBElement.value == 0 ) {
         outNBElement.value = 0;
         return retCode ;
      }
      maBegIdx = (int)outBegIdx.value;
      retCode = stdDevUnguarded((int)outBegIdx.value, endIdx, inReal, optInTimePeriod, 1.0, outBegIdx, outNBElement, tempBuffer2);
      if( retCode != RetCode.Success ) {
         outNBElement.value = 0;
         return retCode ;
      }
      if( (int)outBegIdx.value > maBegIdx ) {
         shiftIdx = (int)outBegIdx.value - maBegIdx;
      } else {
         shiftIdx = 0;
      }
      System.arraycopy(tempBuffer1, shiftIdx, outRealMiddleBand, 0, outNBElement.value * 1);
      if( optInNbDevUp == optInNbDevDn ) {
         for( i = 0; i < (int)outNBElement.value; i += 1 ) {
            tempReal = tempBuffer2[i] * optInNbDevUp;
            tempReal2 = outRealMiddleBand[i];
            outRealUpperBand[i] = tempReal2 + tempReal;
            outRealLowerBand[i] = tempReal2 - tempReal;
         }
      } else {
         for( i = 0; i < (int)outNBElement.value; i += 1 ) {
            tempReal2 = outRealMiddleBand[i];
            outRealUpperBand[i] = Math.fma(tempBuffer2[i], optInNbDevUp, tempReal2);
            outRealLowerBand[i] = tempReal2 - tempBuffer2[i] * optInNbDevDn;
         }
      }
      return RetCode.Success ;
   }
