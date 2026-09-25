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

int bbw_lookback(int optInTimePeriod, double optInNbDevUp, double optInNbDevDn, TA_MAType optInMAType)
{
   return bbands_lookback( optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType );
}

TA_RetCode bbw(int startIdx, int endIdx,
   const double inReal[],
   int optInTimePeriod,
   double optInNbDevUp,
   double optInNbDevDn,
   TA_MAType optInMAType,
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   TA_RetCode retCode;
   int i, maBegIdx, maNbElement, offset;
   double middle, deviation, tempReal, upper, lower;
   double *tempBuffer;

   if( optInMAType == TA_MAType_SMA )
   {
      /* Keep the middle band's divide in the recurrence, where the running
       * sums' dependency chain hides it; everything downstream of the
       * variance, the square root included, belongs in the tile pass, where
       * it vectorizes.
       */
      double _mid[256];
      double maTotal, shift, varTotal1, varTotal2, meanValue1, variance, _invPeriod, _tempReal, _peakTotal2;
      int _i, _j, _k, _outIdx, _tileBase, _tileEnd, _trailingIdx, _windowStart, _lookbackTotal, _barsSinceReseed;

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
      _peakTotal2 = varTotal2;
      do
      {
         if( endIdx - _i > 255 )
            _tileEnd = _i + 255;
         else
            _tileEnd = endIdx;
         _tileBase = _outIdx;
         do
         {
            maTotal += inReal[_i];
            _tempReal = inReal[_i] - shift;
            varTotal1 += _tempReal;
            _tempReal *= _tempReal;
            varTotal2 += _tempReal;
            _peakTotal2 = ( varTotal2 > _peakTotal2 ) ? varTotal2 : _peakTotal2;

            meanValue1 = varTotal1 * _invPeriod;
            variance = varTotal2 * _invPeriod - meanValue1 * meanValue1;
            _mid[_outIdx - _tileBase] = maTotal / optInTimePeriod;

            maTotal -= inReal[_trailingIdx];
            _tempReal = inReal[_trailingIdx] - shift;
            varTotal1 -= _tempReal;
            _tempReal *= _tempReal;
            varTotal2 -= _tempReal;
            _trailingIdx++;

            _barsSinceReseed--;
            if( variance < 0.000001 * ( _peakTotal2 * _invPeriod )
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
               if( variance < 0.000001 * ( varTotal2 * _invPeriod ) )
               {
                  shift = inReal[_i];
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
               }
               _peakTotal2 = varTotal2;
               if( variance < 0.000000000001 * ( varTotal2 * _invPeriod ) )
                  variance = 0.0;
               _tempReal = inReal[_windowStart] - shift;
               varTotal1 -= _tempReal;
               _tempReal *= _tempReal;
               varTotal2 -= _tempReal;
            }

            /* outReal may be inReal: at startIdx == optInTimePeriod-1 this
             * slot is the first one the rebuild above reads. */
            outReal[_outIdx] = variance;
            _outIdx++;
            _i++;
         } while( _i <= _tileEnd );

         /* Each band is rounded as TA_BBANDS rounds it and the width is taken
          * from the two rounded bands, which keeps BBW bit-identical to
          * (upper - lower) / middle over TA_BBANDS' outputs.
          *
          * Store, then overwrite: gcc keeps a guarded or selected quotient
          * scalar under -ftrapping-math, and says nothing.
          */
         if( optInNbDevUp == optInNbDevDn )
         {
            for( _k=0; _k < _outIdx - _tileBase; _k++ )
            {
               middle   = _mid[_k];
               tempReal = sqrt(outReal[_tileBase+_k]) * optInNbDevUp;
               upper    = middle + tempReal;
               lower    = middle - tempReal;
               outReal[_tileBase+_k] = (upper - lower) / middle;
               if( middle == 0.0 )
                  outReal[_tileBase+_k] = 0.0;
            }
         }
         else
         {
            for( _k=0; _k < _outIdx - _tileBase; _k++ )
            {
               middle    = _mid[_k];
               deviation = sqrt(outReal[_tileBase+_k]);
               upper     = middle + (deviation * optInNbDevUp);
               lower     = middle - (deviation * optInNbDevDn);
               outReal[_tileBase+_k] = (upper - lower) / middle;
               if( middle == 0.0 )
                  outReal[_tileBase+_k] = 0.0;
            }
         }
      } while( _i <= endIdx );

      *outNBElement = _outIdx;
      *outBegIdx = startIdx;
      return TA_SUCCESS;
   }

   if( bbw_lookback( optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType ) > endIdx )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }

   tempBuffer = malloc((endIdx-startIdx+1) * sizeof(double));
   if( !tempBuffer )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_ALLOC_ERR;
   }

   /* Before the variance: it may be written over inReal. */
   retCode = ma( startIdx, endIdx, inReal,
      optInTimePeriod, optInMAType,
      &maBegIdx, &maNbElement, tempBuffer );
   if( retCode != TA_SUCCESS )
   {
      free( tempBuffer );
      *outBegIdx = 0;
      *outNBElement = 0;
      return retCode;
   }

   /* From the moving average's begIdx, as TA_BBANDS enters its deviation:
    * the variance's shift and reseed schedule are anchored on its start. */
   retCode = var( maBegIdx, endIdx, inReal,
      optInTimePeriod, 1.0,
      outBegIdx, outNBElement, outReal );
   if( retCode != TA_SUCCESS )
   {
      free( tempBuffer );
      *outBegIdx = 0;
      *outNBElement = 0;
      return retCode;
   }

   offset = maNbElement - *outNBElement;

   if( optInNbDevUp == optInNbDevDn )
   {
      for( i=0; i < (int)*outNBElement; i++ )
      {
         middle   = tempBuffer[i+offset];
         tempReal = sqrt(outReal[i]) * optInNbDevUp;
         upper    = middle + tempReal;
         lower    = middle - tempReal;
         outReal[i] = (upper - lower) / middle;
         if( middle == 0.0 )
            outReal[i] = 0.0;
      }
   }
   else
   {
      for( i=0; i < (int)*outNBElement; i++ )
      {
         middle    = tempBuffer[i+offset];
         deviation = sqrt(outReal[i]);
         upper     = middle + (deviation * optInNbDevUp);
         lower     = middle - (deviation * optInNbDevDn);
         outReal[i] = (upper - lower) / middle;
         if( middle == 0.0 )
            outReal[i] = 0.0;
      }
   }

   free( tempBuffer );

   return TA_SUCCESS;
}

/* The base with every width map over one index and no guard, the only shape
 * of these maps C2 vectorizes: the SMA block maps a tile and copies it out,
 * the general path first shifts the middle band onto the variance's index.
 * Only Java takes it: the tile's extra pass costs the other backends, and in
 * Rust and C# the copy's call spills the recurrence. A change to one body goes
 * into both.
 */
/* PRAGMA TA_ALT={BATCH,JAVA} */
TA_RetCode bbw_ALT1(int startIdx, int endIdx,
   const double inReal[],
   int optInTimePeriod,
   double optInNbDevUp,
   double optInNbDevDn,
   TA_MAType optInMAType,
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   TA_RetCode retCode;
   int i, maBegIdx, maNbElement, offset;
   double middle, deviation, tempReal, upper, lower;
   double *tempBuffer;

   if( optInMAType == TA_MAType_SMA )
   {
      double _mid[256];
      double _var[256];
      double maTotal, shift, varTotal1, varTotal2, meanValue1, variance, _invPeriod, _tempReal, _peakTotal2;
      int _i, _j, _k, _t, _zero, _outIdx, _tileEnd, _trailingIdx, _windowStart, _lookbackTotal, _barsSinceReseed;

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
      _peakTotal2 = varTotal2;
      do
      {
         if( endIdx - _i > 255 )
            _tileEnd = _i + 255;
         else
            _tileEnd = endIdx;
         _t = 0;
         _zero = 0;
         do
         {
            maTotal += inReal[_i];
            _tempReal = inReal[_i] - shift;
            varTotal1 += _tempReal;
            _tempReal *= _tempReal;
            varTotal2 += _tempReal;
            _peakTotal2 = ( varTotal2 > _peakTotal2 ) ? varTotal2 : _peakTotal2;

            meanValue1 = varTotal1 * _invPeriod;
            variance = varTotal2 * _invPeriod - meanValue1 * meanValue1;
            _mid[_t] = maTotal / optInTimePeriod;
            if( _mid[_t] == 0.0 )
               _zero = 1;

            maTotal -= inReal[_trailingIdx];
            _tempReal = inReal[_trailingIdx] - shift;
            varTotal1 -= _tempReal;
            _tempReal *= _tempReal;
            varTotal2 -= _tempReal;
            _trailingIdx++;

            _barsSinceReseed--;
            if( variance < 0.000001 * ( _peakTotal2 * _invPeriod )
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
               if( variance < 0.000001 * ( varTotal2 * _invPeriod ) )
               {
                  shift = inReal[_i];
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
               }
               _peakTotal2 = varTotal2;
               if( variance < 0.000000000001 * ( varTotal2 * _invPeriod ) )
                  variance = 0.0;
               _tempReal = inReal[_windowStart] - shift;
               varTotal1 -= _tempReal;
               _tempReal *= _tempReal;
               varTotal2 -= _tempReal;
            }

            _var[_t] = variance;
            _t++;
            _i++;
         } while( _i <= _tileEnd );

         if( optInNbDevUp == optInNbDevDn )
         {
            for( _k=0; _k < _t; _k++ )
            {
               middle   = _mid[_k];
               tempReal = sqrt(_var[_k]) * optInNbDevUp;
               upper    = middle + tempReal;
               lower    = middle - tempReal;
               _var[_k] = (upper - lower) / middle;
            }
         }
         else
         {
            for( _k=0; _k < _t; _k++ )
            {
               middle    = _mid[_k];
               deviation = sqrt(_var[_k]);
               upper     = middle + (deviation * optInNbDevUp);
               lower     = middle - (deviation * optInNbDevDn);
               _var[_k]  = (upper - lower) / middle;
            }
         }

         if( _zero != 0 )
         {
            for( _k=0; _k < _t; _k++ )
            {
               if( _mid[_k] == 0.0 )
                  _var[_k] = 0.0;
            }
         }

         /* outReal may be inReal: a tile is written only after its last read. */
         memcpy( &outReal[_outIdx], _var, _t * sizeof(double) );
         _outIdx += _t;
      } while( _i <= endIdx );

      *outNBElement = _outIdx;
      *outBegIdx = startIdx;
      return TA_SUCCESS;
   }

   if( bbw_lookback( optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType ) > endIdx )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }

   tempBuffer = malloc((endIdx-startIdx+1) * sizeof(double));
   if( !tempBuffer )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_ALLOC_ERR;
   }

   retCode = ma( startIdx, endIdx, inReal,
      optInTimePeriod, optInMAType,
      &maBegIdx, &maNbElement, tempBuffer );
   if( retCode != TA_SUCCESS )
   {
      free( tempBuffer );
      *outBegIdx = 0;
      *outNBElement = 0;
      return retCode;
   }

   retCode = var( maBegIdx, endIdx, inReal,
      optInTimePeriod, 1.0,
      outBegIdx, outNBElement, outReal );
   if( retCode != TA_SUCCESS )
   {
      free( tempBuffer );
      *outBegIdx = 0;
      *outNBElement = 0;
      return retCode;
   }

   offset = maNbElement - *outNBElement;
   if( offset != 0 )
      memmove( tempBuffer, &tempBuffer[offset], *outNBElement * sizeof(double) );

   if( optInNbDevUp == optInNbDevDn )
   {
      for( i=0; i < (int)*outNBElement; i++ )
      {
         middle   = tempBuffer[i];
         tempReal = sqrt(outReal[i]) * optInNbDevUp;
         upper    = middle + tempReal;
         lower    = middle - tempReal;
         outReal[i] = (upper - lower) / middle;
      }
   }
   else
   {
      for( i=0; i < (int)*outNBElement; i++ )
      {
         middle    = tempBuffer[i];
         deviation = sqrt(outReal[i]);
         upper     = middle + (deviation * optInNbDevUp);
         lower     = middle - (deviation * optInNbDevDn);
         outReal[i] = (upper - lower) / middle;
      }
   }

   for( i=0; i < (int)*outNBElement; i++ )
   {
      if( tempBuffer[i] == 0.0 )
         outReal[i] = 0.0;
   }

   free( tempBuffer );

   return TA_SUCCESS;
}
