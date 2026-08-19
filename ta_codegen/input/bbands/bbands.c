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
 *  072426 MF,CC  Lookback is now max(MA, STDDEV) so it is honest for MA types
 *                whose lookback is below the deviation's (MAMA >= 34, and
 *                TA_MAType_DISABLED); required for streaming (issue #93).
 *
 */

int bbands_lookback(int optInTimePeriod, double optInNbDevUp, double optInNbDevDn, TA_MAType optInMAType)
{
   int maLookback;
   int stddevLookback;

   (void)optInNbDevUp;
   (void)optInNbDevDn;

   /* A band value needs BOTH the middle-band moving average and the standard
    * deviation of the outer bands, so the first valid output is the later of the
    * two lookbacks. For every MA type whose lookback is at least
    * optInTimePeriod-1 this equals the MA lookback (unchanged); it rises above it
    * only when the MA lookback is the smaller one - TA_MAType_MAMA at
    * optInTimePeriod >= 34 (constant MAMA lookback 32) and TA_MAType_DISABLED
    * (MA lookback 0, issue #93). Reporting the honest value keeps
    * outBegIdx == lookback: issue #99 kept it at the MA lookback, which
    * under-reported those cases (outBegIdx > lookback) and broke streaming, whose
    * Open is tied to lookback+1. The middle band still begins at the MA's earlier
    * begIdx internally and is realigned to this later bar in bbands() below.
    */
   maLookback = ma_lookback( optInTimePeriod, optInMAType );
   stddevLookback = stddev_lookback( optInTimePeriod, 1.0 );
   return maLookback > stddevLookback ? maLookback : stddevLookback;
}

TA_RetCode bbands(int startIdx, int endIdx,
   const double inReal[],
   int optInTimePeriod,
   double optInNbDevUp,
   double optInNbDevDn,
   TA_MAType optInMAType,
   int *outBegIdx, int *outNBElement,
   double outRealUpperBand[],
   double outRealMiddleBand[],
   double outRealLowerBand[])
{
   TA_RetCode retCode;
   int i;
   int maBegIdx;
   int shiftIdx;
   double tempReal, tempReal2;
   double *tempBuffer1;
   double *tempBuffer2;

   if( optInMAType == TA_MAType_SMA )
   {
      /* SMA fast path: the middle band (SMA) and the standard deviation share one
       * pass over the window below. Bit-identical to the general MA + STDDEV path
       * (which the stream composes for every MA type).
       *
       * Identify TWO temporary buffers among the outputs so the calculation
       * needs no memory allocation; whenever possible make tempBuffer1 be the
       * middle band output, saving one copy operation.
       */
      if( inReal == outRealUpperBand )
      {
         tempBuffer1 = outRealMiddleBand;
         tempBuffer2 = outRealLowerBand;
      }
      else if( inReal == outRealLowerBand )
      {
         tempBuffer1 = outRealMiddleBand;
         tempBuffer2 = outRealUpperBand;
      }
      else if( inReal == outRealMiddleBand )
      {
         tempBuffer1 = outRealLowerBand;
         tempBuffer2 = outRealUpperBand;
      }
      else
      {
         tempBuffer1 = outRealMiddleBand;
         tempBuffer2 = outRealUpperBand;
      }
      /* Check that the caller is not doing tricky things.
       * (like using the input buffer in two output!)
       */
      if( (tempBuffer1 == inReal) || (tempBuffer2 == inReal) )
         return TA_BAD_PARAM;

      /* One pass with two independent recurrences: the SMA running sum (maTotal,
       * mean -> tempBuffer1, bit-identical to TA_MA(SMA)) and the shifted-data
       * variance (-> tempBuffer2, bit-identical to TA_STDDEV/TA_VAR - see var.c).
       * The variance carries its own shift and reseed; the SMA sum is untouched by
       * it. tempBuffer1/2 never alias inReal (checked above). */
      {
         double maTotal, shift, varTotal1, varTotal2, meanValue1, variance, _invPeriod, _tempReal;
         int _i, _j, _outIdx, _trailingIdx, _windowStart, _lookbackTotal, _barsSinceReseed;

         _lookbackTotal = optInTimePeriod - 1;
         if( startIdx < _lookbackTotal )
            startIdx = _lookbackTotal;

         if( startIdx > endIdx )
         {
            *outBegIdx = 0;
            *outNBElement = 0;
            return TA_SUCCESS;
         }

         _invPeriod = 1.0 / (double)optInTimePeriod;
         _trailingIdx = startIdx - _lookbackTotal;
         shift = inReal[_trailingIdx];

         maTotal = 0.0;
         varTotal1 = 0.0;
         varTotal2 = 0.0;
         for( _j=_trailingIdx; _j < startIdx; _j++ )
         {
            maTotal += inReal[_j];
            _tempReal = inReal[_j] - shift;
            varTotal1 += _tempReal;
            _tempReal *= _tempReal;
            varTotal2 += _tempReal;
         }

         _i = startIdx;
         _outIdx = 0;
         _barsSinceReseed = 32 * optInTimePeriod;
         do
         {
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
            _trailingIdx++;

            _barsSinceReseed--;
            if( variance < 0.000001 * ( varTotal2 * _invPeriod )
               || _tempReal > 1000000.0 * varTotal2
               || _barsSinceReseed <= 0 )
            {
               _barsSinceReseed = 32 * optInTimePeriod;
               _windowStart = _i - _lookbackTotal;
               _tempReal = 0.0;
               for( _j=_windowStart; _j <= _i; _j++ )
                  _tempReal += inReal[_j];
               shift = _tempReal * _invPeriod;
               varTotal1 = 0.0;
               varTotal2 = 0.0;
               for( _j=_windowStart; _j <= _i; _j++ )
               {
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

            if( !TA_IS_ZERO_OR_NEG(variance) )
               tempBuffer2[_outIdx] = sqrt(variance);
            else
               tempBuffer2[_outIdx] = 0.0;
            _outIdx++;
            _i++;
         } while( _i <= endIdx );

         *outNBElement = _outIdx;
         *outBegIdx = startIdx;
      }

      /* Copy the MA calculation into the middle band ouput, unless
       * the calculation was done into it already!
       */
      if( tempBuffer1 != outRealMiddleBand )
      {
         memcpy(outRealMiddleBand, tempBuffer1, (*outNBElement) * sizeof(double));
      }

      /* Now do a tight loop to calculate the upper/lower band at the same time. */
      if( optInNbDevUp == optInNbDevDn )
      {
         for( i=0; i < (int)*outNBElement; i++ )
         {
            tempReal  = tempBuffer2[i] * optInNbDevUp;
            tempReal2 = outRealMiddleBand[i];
            outRealUpperBand[i] = tempReal2 + tempReal;
            outRealLowerBand[i] = tempReal2 - tempReal;
         }
      }
      else
      {
         for( i=0; i < (int)*outNBElement; i++ )
         {
            tempReal  = tempBuffer2[i];
            tempReal2 = outRealMiddleBand[i];
            outRealUpperBand[i] = tempReal2 + (tempReal * optInNbDevUp);
            outRealLowerBand[i] = tempReal2 - (tempReal * optInNbDevDn);
         }
      }

      return TA_SUCCESS;
   }

   /* General path (every MA type other than SMA): the middle band is the moving
    * average and the deviation is the standard deviation of the input, combined
    * at the same bar. Two intermediate buffers are allocated so the input may
    * safely alias an output (it is only read here).
    */

   /* Nothing to produce: the range is shorter than the lookback. Return before
    * touching anything.
    *
    * Without this the moving average below runs first, and for the MA types whose
    * lookback is BELOW the deviation's - TA_MAType_DISABLED (MA lookback 0) and
    * TA_MAType_MAMA at optInTimePeriod >= 34 - it reads the whole range and
    * computes a middle band the empty standard deviation then discards.
    * Observably identical (the empty deviation already yields 0,0 here), but it
    * is the difference between "a range shorter than the lookback reads nothing"
    * being true of this function and being false: with a caller-supplied inReal
    * that stops short of endIdx, that discarded work is an out-of-bounds read.
    * The SMA fast path above needs no such guard - its own lookback IS the
    * deviation's, so its clamp already covers it. Pinned by the zero-length
    * no-I/O probe over every guarded core, at every MA type.
    */
   if( bbands_lookback( optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType ) > endIdx )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }

   tempBuffer1 = malloc((endIdx-startIdx+1) * sizeof(double));
   if( !tempBuffer1 )
      return TA_ALLOC_ERR;
   tempBuffer2 = malloc((endIdx-startIdx+1) * sizeof(double));
   if( !tempBuffer2 )
   {
      free( tempBuffer1 );
      return TA_ALLOC_ERR;
   }

   /* Calculate the middle band moving average. */
   retCode = ma( startIdx, endIdx, inReal,
      optInTimePeriod, optInMAType,
      outBegIdx, outNBElement, tempBuffer1 );

   if( (retCode != TA_SUCCESS ) || ((int)*outNBElement == 0) )
   {
      *outNBElement = 0;
      free( tempBuffer1 );
      free( tempBuffer2 );
      return retCode;
   }

   /* Remember where the moving average begins, to realign it below. */
   maBegIdx = (int)*outBegIdx;

   /* Calculate the Standard Deviation into tempBuffer2. */
   retCode = stddev( (int)*outBegIdx, endIdx, inReal,
      optInTimePeriod, 1.0,
      outBegIdx, outNBElement, tempBuffer2 );

   if( retCode != TA_SUCCESS )
   {
      *outNBElement = 0;
      free( tempBuffer1 );
      free( tempBuffer2 );
      return retCode;
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
   if( (int)*outBegIdx > maBegIdx )
      shiftIdx = (int)*outBegIdx - maBegIdx;
   else
      shiftIdx = 0;
   memmove( outRealMiddleBand, &tempBuffer1[shiftIdx], (*outNBElement) * sizeof(double) );

   /* Now do a tight loop to calculate the upper/lower band at the same time. */
   if( optInNbDevUp == optInNbDevDn )
   {
      for( i=0; i < (int)*outNBElement; i++ )
      {
         tempReal  = tempBuffer2[i] * optInNbDevUp;
         tempReal2 = outRealMiddleBand[i];
         outRealUpperBand[i] = tempReal2 + tempReal;
         outRealLowerBand[i] = tempReal2 - tempReal;
      }
   }
   else
   {
      for( i=0; i < (int)*outNBElement; i++ )
      {
         tempReal2 = outRealMiddleBand[i];
         outRealUpperBand[i] = tempReal2 + (tempBuffer2[i] * optInNbDevUp);
         outRealLowerBand[i] = tempReal2 - (tempBuffer2[i] * optInNbDevDn);
      }
   }

   free( tempBuffer1 );
   free( tempBuffer2 );

   return TA_SUCCESS;
}
