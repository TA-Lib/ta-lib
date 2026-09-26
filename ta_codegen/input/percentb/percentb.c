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

int percentb_lookback(int optInTimePeriod, double optInNbDevUp, double optInNbDevDn, TA_MAType optInMAType)
{
   return bbands_lookback( optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType );
}

TA_RetCode percentb(int startIdx, int endIdx,
   const double inReal[],
   int optInTimePeriod,
   double optInNbDevUp,
   double optInNbDevDn,
   TA_MAType optInMAType,
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   TA_RetCode retCode;
   int i, maBegIdx, maNbElement, xBegIdx, xNbElement, offsetMA, offsetX;
   double middle, deviation, tempReal, upper, lower, den;
   double *tempMA;
   double *tempX;

   if( optInMAType == TA_MAType_SMA )
   {
      /* Keep the middle band's divide in the recurrence, where the running
       * sums' dependency chain hides it; the square root, the bands and the
       * quotient belong in the tile pass, where they vectorize. The tile pass
       * takes x from _x, never from inReal: when outReal is inReal, the
       * variances stored below already cover the tile's x slots.
       */
      double _mid[256];
      double _x[256];
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
            _x[_outIdx - _tileBase] = inReal[_i];
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
          * from the two rounded bands, so a zero width is the one the
          * composition over TA_BBANDS divides by.
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
               den      = upper - lower;
               outReal[_tileBase+_k] = (_x[_k] - lower) / den;
               if( den == 0.0 )
                  outReal[_tileBase+_k] = 0.5;
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
               den       = upper - lower;
               outReal[_tileBase+_k] = (_x[_k] - lower) / den;
               if( den == 0.0 )
                  outReal[_tileBase+_k] = 0.5;
            }
         }
      } while( _i <= endIdx );

      *outNBElement = _outIdx;
      *outBegIdx = startIdx;
      return TA_SUCCESS;
   }

   if( percentb_lookback( optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType ) > endIdx )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }

   tempMA = malloc((endIdx-startIdx+1) * sizeof(double));
   if( !tempMA )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_ALLOC_ERR;
   }
   tempX = malloc((endIdx-startIdx+1) * sizeof(double));
   if( !tempX )
   {
      free( tempMA );
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_ALLOC_ERR;
   }

   /* Both before the variance: it may be written over inReal. */
   retCode = ma( startIdx, endIdx, inReal,
      optInTimePeriod, optInMAType,
      &maBegIdx, &maNbElement, tempMA );
   if( retCode != TA_SUCCESS )
   {
      free( tempMA );
      free( tempX );
      *outBegIdx = 0;
      *outNBElement = 0;
      return retCode;
   }

   retCode = ma( maBegIdx, endIdx, inReal,
      1, optInMAType,
      &xBegIdx, &xNbElement, tempX );
   if( retCode != TA_SUCCESS )
   {
      free( tempMA );
      free( tempX );
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
      free( tempMA );
      free( tempX );
      *outBegIdx = 0;
      *outNBElement = 0;
      return retCode;
   }

   offsetMA = maNbElement - *outNBElement;
   offsetX  = xNbElement - *outNBElement;

   if( optInNbDevUp == optInNbDevDn )
   {
      for( i=0; i < (int)*outNBElement; i++ )
      {
         middle   = tempMA[i+offsetMA];
         tempReal = sqrt(outReal[i]) * optInNbDevUp;
         upper    = middle + tempReal;
         lower    = middle - tempReal;
         den      = upper - lower;
         outReal[i] = (tempX[i+offsetX] - lower) / den;
         if( den == 0.0 )
            outReal[i] = 0.5;
      }
   }
   else
   {
      for( i=0; i < (int)*outNBElement; i++ )
      {
         middle    = tempMA[i+offsetMA];
         deviation = sqrt(outReal[i]);
         upper     = middle + (deviation * optInNbDevUp);
         lower     = middle - (deviation * optInNbDevDn);
         den       = upper - lower;
         outReal[i] = (tempX[i+offsetX] - lower) / den;
         if( den == 0.0 )
            outReal[i] = 0.5;
      }
   }

   free( tempMA );
   free( tempX );

   return TA_SUCCESS;
}

/* The base with every map over one index and no guard, the only shape of these
 * maps C2 vectorizes: the SMA block maps a tile and copies it out, the general
 * path first shifts both series onto the variance's index. The width is parked
 * in the middle band's slot for the guard pass. Only Java takes it. A change to
 * one body goes into all three.
 *
 * The equal-k upper band is written tempReal + middle: spelled middle + tempReal,
 * that map compiles scalar after some JIT histories, silently.
 */
/* PRAGMA TA_ALT={BATCH,JAVA} */
TA_RetCode percentb_ALT1(int startIdx, int endIdx,
   const double inReal[],
   int optInTimePeriod,
   double optInNbDevUp,
   double optInNbDevDn,
   TA_MAType optInMAType,
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   TA_RetCode retCode;
   int i, maBegIdx, maNbElement, xBegIdx, xNbElement, offsetMA, offsetX;
   double middle, deviation, tempReal, upper, lower, den;
   double *tempMA;
   double *tempX;

   if( optInMAType == TA_MAType_SMA )
   {
      double _mid[256];
      double _var[256];
      double _x[256];
      double maTotal, shift, varTotal1, varTotal2, meanValue1, variance, _invPeriod, _tempReal, _peakTotal2;
      int _i, _j, _k, _t, _outIdx, _tileEnd, _trailingIdx, _windowStart, _lookbackTotal, _barsSinceReseed;

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
         do
         {
            _x[_t] = inReal[_i];
            maTotal += inReal[_i];
            _tempReal = inReal[_i] - shift;
            varTotal1 += _tempReal;
            _tempReal *= _tempReal;
            varTotal2 += _tempReal;
            _peakTotal2 = ( varTotal2 > _peakTotal2 ) ? varTotal2 : _peakTotal2;

            meanValue1 = varTotal1 * _invPeriod;
            variance = varTotal2 * _invPeriod - meanValue1 * meanValue1;
            _mid[_t] = maTotal / optInTimePeriod;

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
               upper    = tempReal + middle;
               lower    = middle - tempReal;
               den      = upper - lower;
               _var[_k] = (_x[_k] - lower) / den;
               _mid[_k] = den;
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
               den       = upper - lower;
               _var[_k]  = (_x[_k] - lower) / den;
               _mid[_k]  = den;
            }
         }

         for( _k=0; _k < _t; _k++ )
         {
            if( _mid[_k] == 0.0 )
               _var[_k] = 0.5;
         }

         /* outReal may be inReal: a tile is written only after its last read. */
         memcpy( &outReal[_outIdx], _var, _t * sizeof(double) );
         _outIdx += _t;
      } while( _i <= endIdx );

      *outNBElement = _outIdx;
      *outBegIdx = startIdx;
      return TA_SUCCESS;
   }

   if( percentb_lookback( optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType ) > endIdx )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }

   tempMA = malloc((endIdx-startIdx+1) * sizeof(double));
   if( !tempMA )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_ALLOC_ERR;
   }
   tempX = malloc((endIdx-startIdx+1) * sizeof(double));
   if( !tempX )
   {
      free( tempMA );
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_ALLOC_ERR;
   }

   retCode = ma( startIdx, endIdx, inReal,
      optInTimePeriod, optInMAType,
      &maBegIdx, &maNbElement, tempMA );
   if( retCode != TA_SUCCESS )
   {
      free( tempMA );
      free( tempX );
      *outBegIdx = 0;
      *outNBElement = 0;
      return retCode;
   }

   retCode = ma( maBegIdx, endIdx, inReal,
      1, optInMAType,
      &xBegIdx, &xNbElement, tempX );
   if( retCode != TA_SUCCESS )
   {
      free( tempMA );
      free( tempX );
      *outBegIdx = 0;
      *outNBElement = 0;
      return retCode;
   }

   retCode = var( maBegIdx, endIdx, inReal,
      optInTimePeriod, 1.0,
      outBegIdx, outNBElement, outReal );
   if( retCode != TA_SUCCESS )
   {
      free( tempMA );
      free( tempX );
      *outBegIdx = 0;
      *outNBElement = 0;
      return retCode;
   }

   offsetMA = maNbElement - *outNBElement;
   if( offsetMA != 0 )
      memmove( tempMA, &tempMA[offsetMA], *outNBElement * sizeof(double) );
   offsetX = xNbElement - *outNBElement;
   if( offsetX != 0 )
      memmove( tempX, &tempX[offsetX], *outNBElement * sizeof(double) );

   if( optInNbDevUp == optInNbDevDn )
   {
      for( i=0; i < (int)*outNBElement; i++ )
      {
         middle    = tempMA[i];
         tempReal  = sqrt(outReal[i]) * optInNbDevUp;
         upper     = tempReal + middle;
         lower     = middle - tempReal;
         den       = upper - lower;
         outReal[i] = (tempX[i] - lower) / den;
         tempMA[i] = den;
      }
   }
   else
   {
      for( i=0; i < (int)*outNBElement; i++ )
      {
         middle    = tempMA[i];
         deviation = sqrt(outReal[i]);
         upper     = middle + (deviation * optInNbDevUp);
         lower     = middle - (deviation * optInNbDevDn);
         den       = upper - lower;
         outReal[i] = (tempX[i] - lower) / den;
         tempMA[i] = den;
      }
   }

   for( i=0; i < (int)*outNBElement; i++ )
   {
      if( tempMA[i] == 0.0 )
         outReal[i] = 0.5;
   }

   free( tempMA );
   free( tempX );

   return TA_SUCCESS;
}

/* PRAGMA TA_ALT={STREAM,ALL_LANGUAGES} x from the bar: the base's SMA block rules out a producer loop */
TA_RetCode percentb_ALT2(int startIdx, int endIdx,
   const double inReal[],
   int optInTimePeriod,
   double optInNbDevUp,
   double optInNbDevDn,
   TA_MAType optInMAType,
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   TA_RetCode retCode;
   int i, lookbackTotal, firstIdx, today, outIdx, maBegIdx, maNbElement, offset;
   double middle, deviation, tempReal, upper, lower, den;
   double *tempMA;
   double *tempX;

   lookbackTotal = percentb_lookback( optInTimePeriod, optInNbDevUp, optInNbDevDn, optInMAType );
   firstIdx = startIdx;
   if( firstIdx < lookbackTotal )
      firstIdx = lookbackTotal;

   if( firstIdx > endIdx )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }

   tempX = malloc((endIdx-firstIdx+1) * sizeof(double));
   if( !tempX )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_ALLOC_ERR;
   }
   tempMA = malloc((endIdx-startIdx+1) * sizeof(double));
   if( !tempMA )
   {
      free( tempX );
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_ALLOC_ERR;
   }

   outIdx = 0;
   today = firstIdx;
   while( today <= endIdx )
   {
      tempX[outIdx++] = inReal[today];
      today++;
   }

   retCode = ma( startIdx, endIdx, inReal,
      optInTimePeriod, optInMAType,
      &maBegIdx, &maNbElement, tempMA );
   if( retCode != TA_SUCCESS )
   {
      free( tempX );
      free( tempMA );
      *outBegIdx = 0;
      *outNBElement = 0;
      return retCode;
   }

   retCode = var( maBegIdx, endIdx, inReal,
      optInTimePeriod, 1.0,
      outBegIdx, outNBElement, outReal );
   if( retCode != TA_SUCCESS )
   {
      free( tempX );
      free( tempMA );
      *outBegIdx = 0;
      *outNBElement = 0;
      return retCode;
   }

   offset = maNbElement - *outNBElement;

   if( optInNbDevUp == optInNbDevDn )
   {
      for( i=0; i < (int)*outNBElement; i++ )
      {
         middle   = tempMA[i+offset];
         tempReal = sqrt(outReal[i]) * optInNbDevUp;
         upper    = middle + tempReal;
         lower    = middle - tempReal;
         den      = upper - lower;
         outReal[i] = (tempX[i] - lower) / den;
         if( den == 0.0 )
            outReal[i] = 0.5;
      }
   }
   else
   {
      for( i=0; i < (int)*outNBElement; i++ )
      {
         middle    = tempMA[i+offset];
         deviation = sqrt(outReal[i]);
         upper     = middle + (deviation * optInNbDevUp);
         lower     = middle - (deviation * optInNbDevDn);
         den       = upper - lower;
         outReal[i] = (tempX[i] - lower) / den;
         if( den == 0.0 )
            outReal[i] = 0.5;
      }
   }

   free( tempX );
   free( tempMA );

   return TA_SUCCESS;
}
