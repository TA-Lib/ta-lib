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
#endif

/* Calculate a Simple Moving Average.
 * This is an internal version, parameter are assumed validated.
 * (startIdx and endIdx cannot be -1).
 */
#if !defined( _MANAGED )
TA_RetCode TA_INT_SMA( int    startIdx,
                       int    endIdx,
                       const double *inReal_0,
                       int    optInTimePeriod_0, /* From 1 to 200 */
                       int   *outBegIdx,
                       int   *outNbElement,
                       double      *outReal_0 );
#endif

/* Calculate an Exponential Moving Average.
 * This is an internal version, parameter are assumed validated.
 * (startIdx and endIdx cannot be -1).
 */
#if !defined( _MANAGED )
TA_RetCode TA_INT_EMA( int           startIdx,
                       int           endIdx,
                       const double *inReal_0,
                       int           optInTimePeriod_0, /* From 1 to 200 */
                       double        optInK_1,
                       int          *outBegIdx,
                       int          *outNbElement,
                       double       *outReal_0 );
#endif

/* Calculate a MACD
 * This is an internal version, parameter are assumed validated.
 * (startIdx and endIdx cannot be -1).
 */
#if !defined( _MANAGED )
TA_RetCode TA_INT_MACD( int    startIdx,
                        int    endIdx,
                        const double inReal_0[],
                        int    optInFastPeriod_0, /* From 1 to 200, 0 is fix 12 */
                        int    optInSlowPeriod_1, /* From 1 to 200, 0 is fix 26 */
                        int    optInSignalPeriod_2, /* From 1 to 200 */
                        int   *outBegIdx,
                        int   *outNbElement,
                        double       outRealMACD_0[],
                        double       outRealMACDSignal_1[],
                        double       outRealMACDHist_2[] );
#endif

/* Internal Price Oscillator function.
 *
 * A buffer must be provided for intermediate processing
 * 'tempBuffer' must be of at least (endIdx-startIdx+1)
 */
#if !defined( _MANAGED )
TA_RetCode TA_INT_PO( int    startIdx,
                      int    endIdx,
                      const double *inReal_0,
                      int    optInFastPeriod_0, /* From 1 to 200 */
                      int    optInSlowPeriod_1, /* From 1 to 200 */
                      int    optInMethod_2,
                      int   *outBegIdx,
                      int   *outNbElement,
                      double      *outReal_0,
                      double      *tempBuffer,
                      unsigned int  doPercentageOutput );
#endif

/* Internal variance function. */
#if !defined( _MANAGED )
TA_RetCode TA_INT_VAR( int    startIdx,
                       int    endIdx,
                       const double *inReal_0,
                       int    optInTimePeriod_0,                       
                       int   *outBegIdx,
                       int   *outNbElement,
                       double      *outReal_0 );
#endif

/* A function to calculate a standard deviation.
 *
 * This function allows speed optimization when the
 * moving average series is already calculated.
 */
#if !defined( _MANAGED )
void TA_INT_stddev_using_precalc_ma( const double *inReal,
                                     const double *inMovAvg,
                                     int inMovAvgBegIdx,
                                     int inMovAvgNbElement,
                                     int timePeriod,
                                     double *output );
#endif

#if defined( _MANAGED )
   /* Provides the equivalent of some standard "math.h" function for 
    * the Managed C++ port.
    */
   #define floor Math::Floor
   #define ceil  Math::Ceil
   #define fabs  Math::Abs
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
#if defined( _MANAGED )
   #define HILBERT_VARIABLES(varName) \
         double varName##_Odd  __gc [] = new double __gc [3]; \
         double varName##_Even __gc [] = new double __gc [3]; \
         double varName; \
         double prev_##varName##_Odd; \
         double prev_##varName##_Even; \
         double prev_##varName##_input_Odd; \
         double prev_##varName##_input_Even
#else
   #define HILBERT_VARIABLES(varName) \
         double varName##_Odd[3]; \
         double varName##_Even[3]; \
         double varName; \
         double prev_##varName##_Odd; \
         double prev_##varName##_Even; \
         double prev_##varName##_input_Odd; \
         double prev_##varName##_input_Even
#endif

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

/* Convert a period into the equivalent k:
 *
 *    k = 2 / (period + 1)
 *
 * Useful to calculate the 'k' for TA_INT_EMA().
 */
#define PER_TO_K( per ) ((double)2.0 / ((double)(per + 1)))

#endif
