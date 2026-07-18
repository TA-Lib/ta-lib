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
/**** Streaming API *****/

   /**
    * A live BBANDS stream (unrelated to {@code java.util.stream}): one value per
    * closed bar, bit-identical to {@link Core#bbands} over the same series.
    * Open with {@link Core#bbandsOpen}; there is no close — the handle is
    * ordinary heap state, unreferenced handles are simply garbage-collected.
    * <p>Concurrency: a handle is single-writer — {@code update}, {@code peek},
    * {@code value} and {@code copy} must not race with an {@code update} on
    * the same handle. With no concurrent {@code update}, {@code peek}/
    * {@code value}/{@code copy} never write the handle and may be called
    * concurrently after safe publication. Independent handles (including
    * {@code copy()} results) are fully independent. Do not mutate the owning
    * {@link Core}'s settings while streams opened from it are live.
    * <p>Not serializable by design: to checkpoint, retain the history and
    * re-open — the result is bit-identical by contract.
    */
   public static final class BbandsStream {
      final Core core;
      int optInTimePeriod;
      double optInNbDevUp;
      double optInNbDevDn;
      MAType optInMAType;
      double cur_outRealUpperBand;
      double cur_outRealMiddleBand;
      double cur_outRealLowerBand;
      Value cachedValue;
      MovingAverageStream sub0;
      StdDevStream sub1;

      BbandsStream( Core core ) { this.core = core; }

      BbandsStream( BbandsStream other ) {
         this.core = other.core;
         this.optInTimePeriod = other.optInTimePeriod;
         this.optInNbDevUp = other.optInNbDevUp;
         this.optInNbDevDn = other.optInNbDevDn;
         this.optInMAType = other.optInMAType;
         this.cur_outRealUpperBand = other.cur_outRealUpperBand;
         this.cur_outRealMiddleBand = other.cur_outRealMiddleBand;
         this.cur_outRealLowerBand = other.cur_outRealLowerBand;
         this.cachedValue = other.cachedValue;
         this.sub0 = new MovingAverageStream(other.sub0);
         this.sub1 = new StdDevStream(other.sub1);
      }

      /** One output set, in batch output order. Immutable. */
      public static final class Value {
         public final double realUpperBand;
         public final double realMiddleBand;
         public final double realLowerBand;
         Value( double realUpperBand, double realMiddleBand, double realLowerBand ) {
            this.realUpperBand = realUpperBand;
            this.realMiddleBand = realMiddleBand;
            this.realLowerBand = realLowerBand;
         }
         @Override public String toString() {
            return "Value[" + "realUpperBand=" + realUpperBand + ", " + "realMiddleBand=" + realMiddleBand + ", " + "realLowerBand=" + realLowerBand + "]";
         }
         @Override public boolean equals( Object o ) {
            if( !(o instanceof Value) ) return false;
            Value v = (Value) o;
            return Double.doubleToLongBits(this.realUpperBand) == Double.doubleToLongBits(v.realUpperBand) && Double.doubleToLongBits(this.realMiddleBand) == Double.doubleToLongBits(v.realMiddleBand) && Double.doubleToLongBits(this.realLowerBand) == Double.doubleToLongBits(v.realLowerBand);
         }
         @Override public int hashCode() {
            int h = 17;
            h = 31 * h + Double.hashCode(realUpperBand);
            h = 31 * h + Double.hashCode(realMiddleBand);
            h = 31 * h + Double.hashCode(realLowerBand);
            return h;
         }
      }

      /**
       * Commit one closed bar; always produces the new current value.
       * Never throws after a successful open; never allocates handle state.
       */
      public Value update( double inReal ) {
         core.bbandsStreamStep(this, inReal);
         this.cachedValue = new Value(this.cur_outRealUpperBand, this.cur_outRealMiddleBand, this.cur_outRealLowerBand);
         return this.cachedValue;
      }

      /**
       * Evaluate a forming bar without committing — bit-identical to what the
       * next {@code update} with the same bar would return (it is the same
       * generated code, run on a throwaway copy). Deep-copies the handle state
       * on every call: O(period) for windowed indicators — for hot loops,
       * prefer {@code update} on a {@code copy()}.
       */
      public Value peek( double inReal ) {
         BbandsStream scratch = new BbandsStream(this);
         core.bbandsStreamStep(scratch, inReal);
         return new Value(scratch.cur_outRealUpperBand, scratch.cur_outRealMiddleBand, scratch.cur_outRealLowerBand);
      }

      /**
       * The value at the most recently committed bar — the last history bar
       * right after open, then whatever the latest {@code update} returned.
       * A pure field read; {@code peek} does not change it.
       */
      public Value value() {
         return this.cachedValue;
      }

      /**
       * An independent deep copy of this stream: both evolve separately from
       * here on (the Java rendering of the Rust handle's {@code Clone}).
       */
      public BbandsStream copy() {
         return new BbandsStream(this);
      }
   }
   void bbandsStreamStep( BbandsStream sp, double inReal )
   {
      double tempReal = 0.0;
      double tempReal2 = 0.0;
      double cur_tempBuffer1 = 0.0;
      double cur_tempBuffer2 = 0.0;
      double cur_outRealUpperBand = 0.0;
      double cur_outRealLowerBand = 0.0;
      /* Pipeline the new bar through the sub-streams (batch tail order). */
      cur_tempBuffer1 = sp.sub0.update(inReal);
      cur_tempBuffer2 = sp.sub1.update(inReal);
      /* Combine map (batch tail, per bar). */
      if( sp.optInNbDevUp == sp.optInNbDevDn ) {
         tempReal = cur_tempBuffer2 * sp.optInNbDevUp;
         tempReal2 = cur_tempBuffer1;
         cur_outRealUpperBand = tempReal2 + tempReal;
         cur_outRealLowerBand = tempReal2 - tempReal;
      } else {
         tempReal2 = cur_tempBuffer1;
         cur_outRealUpperBand = Math.fma(cur_tempBuffer2, sp.optInNbDevUp, tempReal2);
         cur_outRealLowerBand = tempReal2 - cur_tempBuffer2 * sp.optInNbDevDn;
      }
      sp.cur_outRealUpperBand = cur_outRealUpperBand;
      sp.cur_outRealMiddleBand = cur_tempBuffer1;
      sp.cur_outRealLowerBand = cur_outRealLowerBand;
   }
   private RetCode bbandsOpenBody( BbandsStream sp, double inReal[], int startIdx, int optInTimePeriod, double optInNbDevUp, double optInNbDevDn, MAType optInMAType )
   {
      RetCode retCode;
      int i = 0;
      int maBegIdx = 0;
      int shiftIdx = 0;
      double tempReal = 0;
      double tempReal2 = 0;
      double[] tempBuffer1;
      double[] tempBuffer2;
      MInteger outBegIdx = new MInteger();
      MInteger outNBElement = new MInteger();
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      if( historyLen < 1 ) {
         return RetCode.BadParam;
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
      double[] sc_outRealUpperBand = new double[historyLen];
      double[] sc_outRealMiddleBand = new double[historyLen];
      double[] sc_outRealLowerBand = new double[historyLen];
      /* General path (every MA type other than SMA): the middle band is the moving
       * average and the deviation is the standard deviation of the input, combined
       * at the same bar. Two intermediate buffers are allocated so the input may
       * safely alias an output (it is only read here).
       */
      tempBuffer1 = new double[(int)((endIdx - startIdx + 1) * 1)];
      tempBuffer2 = new double[(int)((endIdx - startIdx + 1) * 1)];
      /* Calculate the middle band moving average. */
      /* Sub-stream 0: ma over `inReal`, warmed from bar 0 up to the
       * sub-call's own startIdx (the seeding point). */
      MovingAverageStream sub0 = movingAverageOpenInternal(java.util.Arrays.copyOfRange(inReal, 0, (endIdx) + 1), startIdx, optInTimePeriod, optInMAType);
      retCode = movingAverageUnguarded(startIdx, endIdx, inReal, optInTimePeriod, optInMAType, outBegIdx, outNBElement, tempBuffer1);
      if( retCode != RetCode.Success || (int)outNBElement.value == 0 ) {
         outNBElement.value = 0;
         return retCode ;
      }
      /* Remember where the moving average begins, to realign it below. */
      maBegIdx = (int)outBegIdx.value;
      /* Calculate the Standard Deviation into tempBuffer2. */
      /* Sub-stream 1: stddev over `inReal`, warmed from bar 0 up to the
       * sub-call's own startIdx (the seeding point). */
      StdDevStream sub1 = stdDevOpenInternal(java.util.Arrays.copyOfRange(inReal, 0, (endIdx) + 1), (int)outBegIdx.value, optInTimePeriod, 1.0);
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
      System.arraycopy(tempBuffer1, shiftIdx, sc_outRealMiddleBand, 0, outNBElement.value * 1);
      /* Now do a tight loop to calculate the upper/lower band at the same time. */
      if( optInNbDevUp == optInNbDevDn ) {
         for( i = 0; i < (int)outNBElement.value; i += 1 ) {
            tempReal = tempBuffer2[i] * optInNbDevUp;
            tempReal2 = sc_outRealMiddleBand[i];
            sc_outRealUpperBand[i] = tempReal2 + tempReal;
            sc_outRealLowerBand[i] = tempReal2 - tempReal;
         }
      } else {
         for( i = 0; i < (int)outNBElement.value; i += 1 ) {
            tempReal2 = sc_outRealMiddleBand[i];
            sc_outRealUpperBand[i] = Math.fma(tempBuffer2[i], optInNbDevUp, tempReal2);
            sc_outRealLowerBand[i] = tempReal2 - tempBuffer2[i] * optInNbDevDn;
         }
      }
      /* Capture the live producer state + sub handles. */
      if( outNBElement.value < 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      sp.optInTimePeriod = optInTimePeriod;
      sp.optInNbDevUp = optInNbDevUp;
      sp.optInNbDevDn = optInNbDevDn;
      sp.optInMAType = optInMAType;
      sp.sub0 = sub0;
      sp.sub1 = sub1;
      sp.cur_outRealUpperBand = sc_outRealUpperBand[outNBElement.value - 1];
      sp.cur_outRealMiddleBand = sc_outRealMiddleBand[outNBElement.value - 1];
      sp.cur_outRealLowerBand = sc_outRealLowerBand[outNBElement.value - 1];
      sp.cachedValue = new BbandsStream.Value(sp.cur_outRealUpperBand, sp.cur_outRealMiddleBand, sp.cur_outRealLowerBand);
      return RetCode.Success;
   }
   private RetCode bbandsOpenAndFillBody( BbandsStream sp, double inReal[], int optInTimePeriod, double optInNbDevUp, double optInNbDevDn, MAType optInMAType, MInteger outBegIdx, MInteger outNBElement, double outRealUpperBand[], double outRealMiddleBand[], double outRealLowerBand[] )
   {
      RetCode retCode;
      int i = 0;
      int maBegIdx = 0;
      int shiftIdx = 0;
      double tempReal = 0;
      double tempReal2 = 0;
      double[] tempBuffer1;
      double[] tempBuffer2;
      int historyLen = inReal.length;
      int endIdx = historyLen - 1;
      int startIdx = 0;
      if( historyLen < 1 ) {
         return RetCode.BadParam;
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
      if( (Object)outRealUpperBand == (Object)inReal || (Object)outRealMiddleBand == (Object)inReal || (Object)outRealLowerBand == (Object)inReal || (Object)outRealUpperBand == (Object)outRealMiddleBand || (Object)outRealUpperBand == (Object)outRealLowerBand || (Object)outRealMiddleBand == (Object)outRealLowerBand ) {
         return RetCode.BadParam;
      }
      double[] sc_outRealUpperBand = new double[historyLen];
      double[] sc_outRealMiddleBand = new double[historyLen];
      double[] sc_outRealLowerBand = new double[historyLen];
      /* General path (every MA type other than SMA): the middle band is the moving
       * average and the deviation is the standard deviation of the input, combined
       * at the same bar. Two intermediate buffers are allocated so the input may
       * safely alias an output (it is only read here).
       */
      tempBuffer1 = new double[(int)((endIdx - startIdx + 1) * 1)];
      tempBuffer2 = new double[(int)((endIdx - startIdx + 1) * 1)];
      /* Calculate the middle band moving average. */
      /* Sub-stream 0: ma over `inReal`, warmed from bar 0 up to the
       * sub-call's own startIdx (the seeding point). */
      MovingAverageStream sub0 = movingAverageOpenInternal(java.util.Arrays.copyOfRange(inReal, 0, (endIdx) + 1), startIdx, optInTimePeriod, optInMAType);
      retCode = movingAverageUnguarded(startIdx, endIdx, inReal, optInTimePeriod, optInMAType, outBegIdx, outNBElement, tempBuffer1);
      if( retCode != RetCode.Success || (int)outNBElement.value == 0 ) {
         outNBElement.value = 0;
         return retCode ;
      }
      /* Remember where the moving average begins, to realign it below. */
      maBegIdx = (int)outBegIdx.value;
      /* Calculate the Standard Deviation into tempBuffer2. */
      /* Sub-stream 1: stddev over `inReal`, warmed from bar 0 up to the
       * sub-call's own startIdx (the seeding point). */
      StdDevStream sub1 = stdDevOpenInternal(java.util.Arrays.copyOfRange(inReal, 0, (endIdx) + 1), (int)outBegIdx.value, optInTimePeriod, 1.0);
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
      System.arraycopy(tempBuffer1, shiftIdx, sc_outRealMiddleBand, 0, outNBElement.value * 1);
      /* Now do a tight loop to calculate the upper/lower band at the same time. */
      if( optInNbDevUp == optInNbDevDn ) {
         for( i = 0; i < (int)outNBElement.value; i += 1 ) {
            tempReal = tempBuffer2[i] * optInNbDevUp;
            tempReal2 = sc_outRealMiddleBand[i];
            sc_outRealUpperBand[i] = tempReal2 + tempReal;
            sc_outRealLowerBand[i] = tempReal2 - tempReal;
         }
      } else {
         for( i = 0; i < (int)outNBElement.value; i += 1 ) {
            tempReal2 = sc_outRealMiddleBand[i];
            sc_outRealUpperBand[i] = Math.fma(tempBuffer2[i], optInNbDevUp, tempReal2);
            sc_outRealLowerBand[i] = tempReal2 - tempBuffer2[i] * optInNbDevDn;
         }
      }
      /* Capture the live producer state + sub handles. */
      if( outNBElement.value < 1 ) {
         return RetCode.OutOfRangeEndIndex;
      }
      sp.optInTimePeriod = optInTimePeriod;
      sp.optInNbDevUp = optInNbDevUp;
      sp.optInNbDevDn = optInNbDevDn;
      sp.optInMAType = optInMAType;
      sp.sub0 = sub0;
      sp.sub1 = sub1;
      sp.cur_outRealUpperBand = sc_outRealUpperBand[outNBElement.value - 1];
      sp.cur_outRealMiddleBand = sc_outRealMiddleBand[outNBElement.value - 1];
      sp.cur_outRealLowerBand = sc_outRealLowerBand[outNBElement.value - 1];
      sp.cachedValue = new BbandsStream.Value(sp.cur_outRealUpperBand, sp.cur_outRealMiddleBand, sp.cur_outRealLowerBand);
      System.arraycopy(sc_outRealUpperBand, 0, outRealUpperBand, 0, outNBElement.value);
      System.arraycopy(sc_outRealMiddleBand, 0, outRealMiddleBand, 0, outNBElement.value);
      System.arraycopy(sc_outRealLowerBand, 0, outRealLowerBand, 0, outNBElement.value);
      return RetCode.Success;
   }
   /* Internal startIdx-anchored open behind bbandsOpen (composition seam). */
   BbandsStream bbandsOpenInternal( double inReal[], int startIdx, int optInTimePeriod, double optInNbDevUp, double optInNbDevDn, MAType optInMAType )
   {
      BbandsStream sp = new BbandsStream(this);
      RetCode retCode = bbandsOpenBody(sp, inReal, startIdx, optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_BBANDS open: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_BBANDS open: internal error");
      }
      throw new IllegalArgumentException("TA_BBANDS open: " + retCode);
   }
   /**
    * Open a live BBANDS stream over the warm-up history; the handle's
    * {@code value()} starts at the last history bar's value — bit-identical
    * to {@link Core#bbands} at that bar.
    * <p>The history must hold at least {@code bbandsLookback(...) + 1} bars
    * (unstable-period aware), or {@link InsufficientHistoryException} is
    * thrown. Out-of-range parameters throw {@link IllegalArgumentException}
    * ({@code Integer.MIN_VALUE} selects an integer parameter's documented
    * default, as in the batch API).
    */
   public BbandsStream bbandsOpen( double inReal[], int optInTimePeriod, double optInNbDevUp, double optInNbDevDn, MAType optInMAType )
   {
      return bbandsOpenInternal(inReal, 0, optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType);
   }
   /**
    * {@link Core#bbandsOpen} that also fills the output array(s) bit-identically
    * to {@link Core#bbands} over the whole history in the same single pass
    * (no separate batch call needed for the warm-up plot). Output arrays must
    * not alias the inputs or each other, and must hold
    * {@code historyLen - lookback} values.
    */
   public BbandsStream bbandsOpenAndFill( double inReal[], int optInTimePeriod, double optInNbDevUp, double optInNbDevDn, MAType optInMAType, MInteger outBegIdx, MInteger outNBElement, double outRealUpperBand[], double outRealMiddleBand[], double outRealLowerBand[] )
   {
      BbandsStream sp = new BbandsStream(this);
      RetCode retCode = bbandsOpenAndFillBody(sp, inReal, optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType, outBegIdx, outNBElement, outRealUpperBand, outRealMiddleBand, outRealLowerBand);
      if( retCode == RetCode.Success ) {
         return sp;
      }
      if( retCode == RetCode.OutOfRangeEndIndex ) {
         throw new InsufficientHistoryException("TA_BBANDS openAndFill: history shorter than lookback + 1");
      }
      if( retCode == RetCode.InternalError ) {
         throw new IllegalStateException("TA_BBANDS openAndFill: internal error");
      }
      throw new IllegalArgumentException("TA_BBANDS openAndFill: " + retCode);
   }
