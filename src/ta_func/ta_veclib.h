#ifndef TA_VECLIB_H
#define TA_VECLIB_H

#if defined(TA_USE_ACCELERATE)
#include <vecLib/vDSP.h>
#include <vecLib/vForce.h>

/*
 * Accelerate dispatch macros for single-input vForce functions
 * (vvacos, vvsin, vvsqrt, etc.) and dual-input vDSP functions
 * (vDSP_vaddD, vDSP_vmulD, etc.).
 *
 * Each macro returns early with the vectorized result when
 * TA_USE_ACCELERATE is defined and the input is double-precision.
 * When the macro is a no-op (single-precision or non-Apple),
 * control falls through to the scalar loop below.
 */

/* Single-input vForce: fn(outReal, inReal+start, &n) */
#define ACCEL_VFORCE_1IN(fn) \
   { \
      int ta_n = endIdx - startIdx + 1; \
      fn(outReal, inReal + startIdx, &ta_n); \
      VALUE_HANDLE_DEREF(outNBElement) = ta_n; \
      VALUE_HANDLE_DEREF(outBegIdx)    = startIdx; \
      return ENUM_VALUE(RetCode,TA_SUCCESS,Success); \
   }

/* Dual-input vDSP: fn(A, 1, B, 1, C, 1, n) — A op B */
#define ACCEL_VDSP_2IN(fn) \
   { \
      const vDSP_Length ta_n = (vDSP_Length)(endIdx - startIdx + 1); \
      fn(inReal0 + startIdx, 1, inReal1 + startIdx, 1, outReal, 1, ta_n); \
      VALUE_HANDLE_DEREF(outNBElement) = (int)ta_n; \
      VALUE_HANDLE_DEREF(outBegIdx)    = startIdx; \
      return ENUM_VALUE(RetCode,TA_SUCCESS,Success); \
   }

/* Dual-input vDSP with swapped operands: fn(B, 1, A, 1, C, 1, n)
 * vDSP_vsubD computes C[i] = B[i] - A[i], so we pass inReal1 as A
 * to get inReal0 - inReal1. Same pattern for vDSP_vdivD. */
#define ACCEL_VDSP_2IN_SWAP(fn) \
   { \
      const vDSP_Length ta_n = (vDSP_Length)(endIdx - startIdx + 1); \
      fn(inReal1 + startIdx, 1, inReal0 + startIdx, 1, outReal, 1, ta_n); \
      VALUE_HANDLE_DEREF(outNBElement) = (int)ta_n; \
      VALUE_HANDLE_DEREF(outBegIdx)    = startIdx; \
      return ENUM_VALUE(RetCode,TA_SUCCESS,Success); \
   }

#endif /* TA_USE_ACCELERATE */

#endif /* TA_VECLIB_H */
