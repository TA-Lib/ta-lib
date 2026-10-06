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

int ppo_lookback(int optInFastPeriod, int optInSlowPeriod, TA_MAType optInMAType)
{
   /* Lookback is driven by the slowest MA. */
   return ma_lookback( max(optInSlowPeriod,optInFastPeriod), optInMAType );
}

TA_RetCode ppo(int startIdx, int endIdx,
   const double inReal[],
   int optInFastPeriod,
   int optInSlowPeriod,
   TA_MAType optInMAType,
   int *outBegIdx, int *outNBElement,
   double outReal[])
{
   double *tempBuffer;
   TA_RetCode retCode;
   double tempReal;
   int tempInteger;
   int fastBeg, fastNb;
   int offset;
   int slowLookback;
   int windowed;
   int zeroRun;
   int i;

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
   if( ma_lookback( max(optInSlowPeriod,optInFastPeriod), optInMAType ) > endIdx )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_SUCCESS;
   }

   if( optInMAType == TA_MAType_SMA )
   {
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
      double _fastTotal, _slowTotal, _fastValue, _slowValue, _slowMA;
      int _i, _j, _outIdx, _fastStart, _slowStart, _fastTrailing, _slowTrailing;
      int _slowLookback, _zeroRun;

      /* Make sure slow is really slower than the fast period! if not, swap... */
      if( optInSlowPeriod < optInFastPeriod )
      {
         tempInteger     = optInSlowPeriod;
         optInSlowPeriod = optInFastPeriod;
         optInFastPeriod = tempInteger;
      }

      _fastStart = optInFastPeriod - 1;
      if( _fastStart < startIdx )
         _fastStart = startIdx;
      _slowStart = optInSlowPeriod - 1;
      if( _slowStart < startIdx )
         _slowStart = startIdx;

      _fastTrailing = _fastStart - (optInFastPeriod - 1);
      _fastTotal = 0.0;
      for( _j=_fastTrailing; _j < _fastStart; _j++ )
         _fastTotal += inReal[_j];

      /* One loop seeds the slow sum and warms the dead-window counter: the bars
       * it walks, [_slowStart-_slowLookback, _slowStart), are exactly the ones
       * the general path warms _zeroRun over.
       */
      _slowLookback = optInSlowPeriod - 1;
      _zeroRun = 0;
      _slowTrailing = _slowStart - _slowLookback;
      _slowTotal = 0.0;
      for( _j=_slowTrailing; _j < _slowStart; _j++ )
      {
         _slowTotal += inReal[_j];
         _zeroRun = fabs(inReal[_j]) <= 0.0 ? _zeroRun + 1 : 0;
      }

      /* The bars the fast MA has and the slow one does not: advance the fast sum
       * alone. No output, but the sum must arrive at _slowStart along the same
       * path TA_SMA would have taken.
       */
      for( _i=_fastStart; _i < _slowStart; _i++ )
      {
         _fastTotal += inReal[_i];
         _fastTotal -= inReal[_fastTrailing];
         _fastTrailing++;
      }

      _outIdx = 0;
      for( _i=_slowStart; _i <= endIdx; _i++ )
      {
         _zeroRun = fabs(inReal[_i]) <= 0.0 ? _zeroRun + 1 : 0;

         _fastTotal += inReal[_i];
         _fastValue = _fastTotal;
         _fastTotal -= inReal[_fastTrailing];
         _fastTrailing++;

         _slowTotal += inReal[_i];
         _slowValue = _slowTotal;
         _slowTotal -= inReal[_slowTrailing];
         _slowTrailing++;

         _slowMA = _slowValue / (double)optInSlowPeriod;
         if( _zeroRun > _slowLookback )
         {
            _zeroRun = _slowLookback;
            outReal[_outIdx] = 0.0;
         }
         else if( !TA_IS_ZERO(_slowMA) )
            outReal[_outIdx] = ((_fastValue / (double)optInFastPeriod - _slowMA)
            / _slowMA) * 100.0;
         else
            outReal[_outIdx] = 0.0;
         _outIdx++;
      }

      *outBegIdx = _slowStart;
      *outNBElement = _outIdx;
      return TA_SUCCESS;
   }

   if( optInMAType == TA_MAType_EMA )
   {
      /* EMA fast path: both recursions in one loop, no buffer. Bit-identical to
       * the general path only while each EMA is seeded at its OWN lookback and
       * keeps ema.c's recursion spelling: the fast EMA starts earlier than the
       * slow one, and a shared seed bar would change every output.
       */
      double _eFastK, _eSlowK, _eFast, _eSlow, _eX;
      double _eFastBeta, _eSlowBeta;
      int _eN, _eToday, _eFastToday, _eSlowToday, _eSlowStart, _eOutIdx;

      if( optInSlowPeriod < optInFastPeriod )
      {
         tempInteger     = optInSlowPeriod;
         optInSlowPeriod = optInFastPeriod;
         optInFastPeriod = tempInteger;
      }

      _eFastBeta = ((double)(optInFastPeriod - 1)) / ((double)(optInFastPeriod + 1));
      _eFastK = 1.0 - _eFastBeta;
      if( _eFastBeta < 0.5 ) _eFastBeta = 1.0 - _eFastK;
      _eSlowBeta = ((double)(optInSlowPeriod - 1)) / ((double)(optInSlowPeriod + 1));
      _eSlowK = 1.0 - _eSlowBeta;
      if( _eSlowBeta < 0.5 ) _eSlowBeta = 1.0 - _eSlowK;

      _eFastToday = ema_lookback( optInFastPeriod );
      if( _eFastToday < startIdx )
         _eFastToday = startIdx;
      _eFastToday -= ema_lookback( optInFastPeriod );
      _eSlowStart = ema_lookback( optInSlowPeriod );
      if( _eSlowStart < startIdx )
         _eSlowStart = startIdx;
      _eSlowToday = _eSlowStart - ema_lookback( optInSlowPeriod );

      _eFast = 0.0;
      for( _eN = 0; _eN < optInFastPeriod; _eN++ )
         _eFast += inReal[_eFastToday++];
      _eFast = _eFast / optInFastPeriod;
      while( _eFastToday <= _eSlowStart )
         _eFast = _eFastK * inReal[_eFastToday++] + _eFastBeta * _eFast;

      _eSlow = 0.0;
      for( _eN = 0; _eN < optInSlowPeriod; _eN++ )
         _eSlow += inReal[_eSlowToday++];
      _eSlow = _eSlow / optInSlowPeriod;
      while( _eSlowToday <= _eSlowStart )
         _eSlow = _eSlowK * inReal[_eSlowToday++] + _eSlowBeta * _eSlow;

      _eOutIdx = 0;
      if( !TA_IS_ZERO(_eSlow) )
         outReal[_eOutIdx] = ((_eFast-_eSlow)/_eSlow)*100.0;
      else
         outReal[_eOutIdx] = 0.0;
      _eOutIdx++;
      _eToday = _eSlowStart + 1;
      while( _eToday <= endIdx )
      {
         _eX = inReal[_eToday++];
         _eFast = _eFastK * _eX + _eFastBeta * _eFast;
         _eSlow = _eSlowK * _eX + _eSlowBeta * _eSlow;
         if( !TA_IS_ZERO(_eSlow) )
            outReal[_eOutIdx] = ((_eFast-_eSlow)/_eSlow)*100.0;
         else
            outReal[_eOutIdx] = 0.0;
         _eOutIdx++;
      }

      *outBegIdx = _eSlowStart;
      *outNBElement = _eOutIdx;
      return TA_SUCCESS;
   }

   /* Allocate an intermediate buffer. */
   tempBuffer = malloc((endIdx-startIdx+1) * sizeof(double));
   if( !tempBuffer )
   {
      *outBegIdx = 0;
      *outNBElement = 0;
      return TA_ALLOC_ERR;
   }

   /* Make sure slow is really slower than
    * the fast period! if not, swap...
    */
   if( optInSlowPeriod < optInFastPeriod )
   {
      /* swap */
      tempInteger     = optInSlowPeriod;
      optInSlowPeriod = optInFastPeriod;
      optInFastPeriod = tempInteger;
   }

   /* Calculate the fast MA into the tempBuffer. */
   retCode = ma( startIdx, endIdx,
      inReal,
      optInFastPeriod,
      optInMAType,
      &fastBeg, &fastNb,
      tempBuffer );
   if( retCode != TA_SUCCESS )
   {
      free( tempBuffer );
      return retCode;
   }

   /* Calculate the slow MA into the output. */
   retCode = ma( startIdx, endIdx,
      inReal,
      optInSlowPeriod,
      optInMAType,
      outBegIdx, outNBElement,
      outReal );
   if( retCode != TA_SUCCESS )
   {
      free( tempBuffer );
      return retCode;
   }

   /* fastNb - *outNBElement == slowBeg - fastBeg (the fast MA has at least as
    * many outputs), so tempBuffer[i+offset] is the fast MA at the same bar as
    * outReal[i], with a non-negative index. An empty slow MA skips the loop.
    */
   offset = fastNb - *outNBElement;

   /* A windowed slow MA (SMA, WMA, TRIMA, HMA) over bars that are all exactly
    * zero is exactly zero, but its running sums leave residue there that
    * TA_IS_ZERO does not catch, and residue over residue is noise where 0 is
    * documented. zeroRun counts the trailing zero bars, held at slowLookback once
    * the window is dead. The recursive MA types really are nonzero on such a
    * window, so they keep the plain loop.
    */
   slowLookback = ma_lookback( optInSlowPeriod, optInMAType );
   windowed = ( optInMAType == TA_MAType_SMA || optInMAType == TA_MAType_WMA ||
      optInMAType == TA_MAType_TRIMA || optInMAType == TA_MAType_HMA ) ? 1 : 0;
   zeroRun = 0;
   for( i = *outBegIdx - slowLookback; i < *outBegIdx; i++ )
      zeroRun = fabs(inReal[i]) <= 0.0 ? zeroRun + 1 : 0;

   if( windowed != 0 )
   {
      for( i=0; i < (int)*outNBElement; i++ )
      {
         zeroRun = fabs(inReal[*outBegIdx + i]) <= 0.0 ? zeroRun + 1 : 0;
         tempReal = outReal[i];
         if( zeroRun > slowLookback )
         {
            zeroRun = slowLookback;
            outReal[i] = 0.0;
         }
         else if( !TA_IS_ZERO(tempReal) )
            outReal[i] = ((tempBuffer[i+offset]-tempReal)/tempReal)*100.0;
         else
            outReal[i] = 0.0;
      }
   }
   else
   {
      /* Calculate ((fast MA)-(slow MA))/(slow MA) in the output. */
      for( i=0; i < (int)*outNBElement; i++ )
      {
         tempReal = outReal[i];
         if( !TA_IS_ZERO(tempReal) )
            outReal[i] = ((tempBuffer[i+offset]-tempReal)/tempReal)*100.0;
         else
            outReal[i] = 0.0;
      }
   }

   free( tempBuffer );

   return TA_SUCCESS;
}
