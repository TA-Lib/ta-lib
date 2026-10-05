#ifndef META_RIDE_H
#define META_RIDE_H

/* The metadata ride: every successful TA_<N>, TA_S_<N> and TA_CallFunc of the
 * suite is held to what the function's own ta_abstract metadata declares.
 * ta_meta_frame.h feeds the first two, the TA_CallFunc guard the third. */

#ifndef TA_ABSTRACT_H
   #include "ta_abstract.h"
#endif
#ifndef TA_ERROR_NUMBER_H
   #include "ta_error_number.h"
#endif

/* site: the function's row in ta_meta_frame.h. single: inputs are float. */
void meta_ride_check( int site, int single, int startIdx, int endIdx,
                      const void *const in[], int nbIn,
                      int outBegIdx, int outNBElement, int lookback,
                      const int shift[], const void *const out[] );

void meta_ride_call( const TA_ParamHolder *params, int startIdx, int endIdx,
                     int outBegIdx, int outNBElement );

/* Mismatches so far. Each printed its own line, up to a cap. */
long meta_ride_mismatches( void );

/* What only a whole unfiltered suite can be held to: every function called,
 * every declared value and sign written at least once, every check live. */
ErrorNumber meta_ride_whole_run( void );

#endif
