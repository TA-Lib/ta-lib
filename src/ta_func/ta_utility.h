/* Provides common mathematical or analysis functions.
 *
 * These functions are all PRIVATE to ta-lib and should
 * never be called directly by the user of the TA-LIB.
 */

#ifndef TA_UTILITY_H
#define TA_UTILITY_H

#if !defined( _MANAGED )
   #ifndef TA_FUNC_H
      #include "ta_func.h"
   #endif

   #ifndef TA_GLOBAL_H
      #include "ta_global.h"
   #endif

/* Calculate a Simple Moving Average.
 * This is an internal version, parameter are assumed validated.
 * (startIdx and endIdx cannot be -1).
 */
TA_RetCode TA_INT_SMA( TA_Integer    startIdx,
                       TA_Integer    endIdx,
                       const double *inReal_0,
                       TA_Integer    optInTimePeriod_0, /* From 1 to 200 */
                       TA_Integer   *outBegIdx,
                       TA_Integer   *outNbElement,
                       double      *outReal_0 );

/* Calculate a Weighted Moving Average.
 * This is an internal version, parameter are assumed validated.
 * (startIdx and endIdx cannot be -1).
 */
TA_RetCode TA_INT_WMA( TA_Integer    startIdx,
                       TA_Integer    endIdx,
                       const double *inReal_0,
                       TA_Integer    optInTimePeriod_0, /* From 1 to 200 */
                       TA_Integer   *outBegIdx,
                       TA_Integer   *outNbElement,
                       double      *outReal_0 );

/* Calculate an Exponential Moving Average.
 * This is an internal version, parameter are assumed validated.
 * (startIdx and endIdx cannot be -1).
 */
TA_RetCode TA_INT_EMA( TA_Integer    startIdx,
                       TA_Integer    endIdx,
                       const double *inReal_0,
                       TA_Integer    optInTimePeriod_0, /* From 1 to 200 */
                       double       optInK_1,
                       TA_Integer   *outBegIdx,
                       TA_Integer   *outNbElement,
                       double      *outReal_0 );

/* Calculate a MACD
 * This is an internal version, parameter are assumed validated.
 * (startIdx and endIdx cannot be -1).
 */
TA_RetCode TA_INT_MACD( TA_Integer    startIdx,
                        TA_Integer    endIdx,
                        const double inReal_0[],
                        TA_Integer    optInFastPeriod_0, /* From 1 to 200, 0 is fix 12 */
                        TA_Integer    optInSlowPeriod_1, /* From 1 to 200, 0 is fix 26 */
                        TA_Integer    optInSignalPeriod_2, /* From 1 to 200 */
                        TA_Integer   *outBegIdx,
                        TA_Integer   *outNbElement,
                        double       outRealMACD_0[],
                        double       outRealMACDSignal_1[],
                        double       outRealMACDHist_2[] );

/* Convert a period into the equivalent k:
 *
 *    k = 2 / (period + 1)
 *
 * Useful to calculate the 'k' for TA_INT_EMA().
 */
#define PER_TO_K( per ) ((double)2.0 / ((double)(per + 1)))

/* Internal Price Oscillator function.
 *
 * A buffer must be provided for intermediate processing
 * 'tempBuffer' must be of at least (endIdx-startIdx+1)
 */
TA_RetCode TA_INT_PO( TA_Integer    startIdx,
                      TA_Integer    endIdx,
                      const double *inReal_0,
                      TA_Integer    optInFastPeriod_0, /* From 1 to 200 */
                      TA_Integer    optInSlowPeriod_1, /* From 1 to 200 */
                      TA_Integer    optInMethod_2,
                      TA_Integer   *outBegIdx,
                      TA_Integer   *outNbElement,
                      double      *outReal_0,
                      double      *tempBuffer,
                      unsigned int  doPercentageOutput );

/* Internal variance function. */
TA_RetCode TA_INT_VAR( TA_Integer    startIdx,
                       TA_Integer    endIdx,
                       const double *inReal_0,
                       TA_Integer    optInTimePeriod_0,                       
                       TA_Integer   *outBegIdx,
                       TA_Integer   *outNbElement,
                       double      *outReal_0 );

/* A particular standard deviation who has the particularity
 * to work with other moving average than the simple moving average.
 *
 * To offer the maximum of flexibility, the caller must pre-calculate the
 * moving average of inReal and pass it as a parameter.
 */
void TA_INT_stddev_using_precalc_ma( const double *inReal,
                                     const double *inMovAvg,
                                     TA_Integer inMovAvgBegIdx,                                    
                                     TA_Integer inMovAvgNbElement,
                                     TA_Integer timePeriod,
                                     double *output );

#endif

/* Rounding macro for doubles. Works only with positive numbers. */
#define round_pos(x) (floor((x)+0.5))

/* Rounding macro for doubles. Works only with negative numbers. */
#define round_neg(x) (ceil((x)-0.5))

/* Rounding with a precision of 2 digit after the dot */
#define round_pos_2(x) ((floor((x*100.0)+0.5))/100.0)
#define round_neg_2(x) ((ceil((x*100.0)-0.5))/100.0)

/* The following macros are being used to do
 * the Hilbert Transform logic as documented
 * in John Ehlers books "Rocket Science For Traders".
 */
#define HILBERT_VARIABLES(varName) \
      double varName##_Odd[3]; \
      double varName##_Even[3]; \
      double varName; \
      double prev_##varName##_Odd; \
      double prev_##varName##_Even; \
      double prev_##varName##_input_Odd; \
      double prev_##varName##_input_Even

#define INIT_HILBERT_VARIABLES(varName) { \
      varName##_Odd [0] = 0.0; \
      varName##_Odd [1] = 0.0; \
      varName##_Odd [2] = 0.0; \
      varName##_Even[0] = 0.0; \
      varName##_Even[1] = 0.0; \
      varName##_Even[2] = 0.0; \
      varName = 0.0; \
      prev_##varName##_Odd        = 0.0; \
      prev_##varName##_Even       = 0.0; \
      prev_##varName##_input_Odd  = 0.0; \
      prev_##varName##_input_Even = 0.0; \
      }

#define DO_HILBERT_TRANSFORM(varName,input,OddOrEvenId) {\
         hilbertTempReal = a * input; \
         varName = -varName##_##OddOrEvenId[hilbertIdx]; \
         varName##_##OddOrEvenId[hilbertIdx] = hilbertTempReal; \
         varName += hilbertTempReal; \
         varName -= prev_##varName##_##OddOrEvenId; \
         prev_##varName##_##OddOrEvenId = b * prev_##varName##_input_##OddOrEvenId; \
         varName += prev_##varName##_##OddOrEvenId; \
         prev_##varName##_input_##OddOrEvenId = input; \
         varName *= adjustedPrevPeriod; \
         }

#define DO_HILBERT_ODD(varName,input)  DO_HILBERT_TRANSFORM(varName,input,Odd)
#define DO_HILBERT_EVEN(varName,input) DO_HILBERT_TRANSFORM(varName,input,Even)

#endif
