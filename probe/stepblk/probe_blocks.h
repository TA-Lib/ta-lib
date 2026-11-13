/* Probe for #548, never shipped: per-callee block updates, each a loop over
 * the callee's private TA_<X>_StepImpl compiled inside the callee's own
 * translation unit (inl_<x>.c). No input validation: the caller owns it.
 * Inputs, outputs and the handle must not overlap. */
#ifndef PROBE_BLOCKS_H
#define PROBE_BLOCKS_H
#include "ta_func.h"

void PROBE_MIDPRICE_Block( TA_MIDPRICE_Stream *restrict sp, const double *restrict high,
                           const double *restrict low, int n, double *restrict out );
void PROBE_SMA_Block( TA_SMA_Stream *restrict sp, const double *restrict in, int n, double *restrict out );
void PROBE_TRANGE_Block( TA_TRANGE_Stream *restrict sp, const double *restrict high, const double *restrict low,
                         const double *restrict close, int n, double *restrict out );
void PROBE_VAR_Block( TA_VAR_Stream *restrict sp, const double *restrict in, int n, double *restrict out );
/* Through STDDEV's own step, which calls the public TA_VAR_Update per bar. */
TA_RetCode PROBE_STDDEV_Block( TA_STDDEV_Stream *restrict sp, const double *restrict in, int n, double *restrict out );
/* PROBE_VAR_Block over the block, then STDDEV's combine map over it. */
void PROBE_STDDEV_BlockSub( TA_STDDEV_Stream *restrict sp, const double *restrict in, int n, double *restrict out );
void PROBE_LINEARREG_Block( TA_LINEARREG_Stream *restrict sp, const double *restrict in, int n, double *restrict out );
void PROBE_EMA_Block( TA_EMA_Stream *restrict sp, const double *restrict in, int n, double *restrict out );

/* Kernels derived mechanically from TA_X_StepImpl (kern_x.h): the step body,
 * statements and order kept, inside a loop over the block; every scalar
 * sp-> field it touches a local loaded before the loop and stored after it
 * (only those written), the ring pointer a local, the ring itself kept; the
 * bar input bound once per bar as a call binds it; no restrict. _KernR is the
 * same text with restrict on sp, in and out. */
void PROBE_SMA_Kern( TA_SMA_Stream *sp, const double *in, int n, double *out );
void PROBE_SMA_KernR( TA_SMA_Stream *restrict sp, const double *restrict in, int n, double *restrict out );
void PROBE_VAR_Kern( TA_VAR_Stream *sp, const double *in, int n, double *out );
void PROBE_VAR_KernR( TA_VAR_Stream *restrict sp, const double *restrict in, int n, double *restrict out );
void PROBE_LINEARREG_Kern( TA_LINEARREG_Stream *sp, const double *in, int n, double *out );
void PROBE_LINEARREG_KernR( TA_LINEARREG_Stream *restrict sp, const double *restrict in, int n, double *restrict out );
void PROBE_EMA_Kern( TA_EMA_Stream *sp, const double *in, int n, double *out );
void PROBE_EMA_KernR( TA_EMA_Stream *restrict sp, const double *restrict in, int n, double *restrict out );
/* PROBE_STDDEV_BlockSub over PROBE_VAR_Kern. */
void PROBE_STDDEV_BlockSubK( TA_STDDEV_Stream *restrict sp, const double *restrict in, int n, double *restrict out );
#endif
