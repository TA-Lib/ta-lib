/* TA_SMA_StepImpl's body by the mechanical rule (probe_blocks.h), included
 * twice by inl_sma.c: KERN_NAME and KERN_R (empty or restrict) pick the copy. */
void KERN_NAME( TA_SMA_Stream *KERN_R sp, const double *KERN_R in, int n, double *KERN_R out )
{
   double cur_outReal = sp->cur_outReal;
   int optInTimePeriod = sp->optInTimePeriod;
   double periodTotal = sp->periodTotal;
   int ringPos_trailingIdx = sp->ringPos_trailingIdx;
   int ringCap_trailingIdx = sp->ringCap_trailingIdx;
   double *ring_trailingIdx_inReal = sp->ring_trailingIdx_inReal;
   int kernBar;

   for( kernBar = 0; kernBar < n; kernBar++ )
   {
      double inReal = in[kernBar];
      double tempReal;
      int ringPosL_trailingIdx;

      if( ringCap_trailingIdx == 0 )
      {
         ring_trailingIdx_inReal[0] = inReal;
      }
      periodTotal += (double)inReal;
      tempReal = periodTotal;
      periodTotal -= (double)ring_trailingIdx_inReal[ringPos_trailingIdx];
      out[kernBar]= tempReal / (double)optInTimePeriod;
      cur_outReal = out[kernBar];
      ringPosL_trailingIdx = ringPos_trailingIdx;
      ring_trailingIdx_inReal[ringPosL_trailingIdx] = inReal;
      ringPosL_trailingIdx = ringPosL_trailingIdx + 1;
      if( ringPosL_trailingIdx >= ringCap_trailingIdx )
      {
         ringPosL_trailingIdx = 0;
      }
      ringPos_trailingIdx = ringPosL_trailingIdx;
   }
   sp->cur_outReal = cur_outReal;
   sp->periodTotal = periodTotal;
   sp->ringPos_trailingIdx = ringPos_trailingIdx;
   sp->outRangeCount += n;
}
