/* DO NOT MODIFY this file.
 * This file is automatically generated by gen_Code.
 *
 * The function define in this file allows to have a consistent
 * framework for calling all the TA function through
 * the TA_CallFunc mechanism.
 *
 * See "ta_abstract.h"
 */

#include "ta_func.h"
#include "ta_frame_priv.h"
#include "ta_frame.h"

/* NEVER CALL directly this function! Use TA_CallFunc. */
 
TA_RetCode TA_ROCR100_FramePP(
                          TA_Integer          startIdx,
                          TA_Integer          endIdx,
                          TA_Integer         *outBegIdx,
                          TA_Integer         *outNbElement,
                          TA_ParamHolderPriv  in[],
                          TA_ParamHolderPriv  optIn[],
                          TA_ParamHolderPriv  out[] )
{
   return TA_ROCR100(
                startIdx,
                endIdx,
                in[0].p.in.data.inReal, /* inReal_0 */
                optIn[0].p.optIn.data.optInInteger, /* optInTimePeriod_0 */
                outBegIdx, 
                outNbElement, 
                out[0].p.out.data.outReal /*  outReal_0 */ );
}

/***************
 * End of File *
 ***************/
