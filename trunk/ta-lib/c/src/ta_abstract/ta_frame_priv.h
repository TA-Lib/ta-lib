/* TA_ParamHolderPriv is the private implementation of a TA_ParamHolder. */

/* Definition in this header shall be used only internaly by the
 * ta_abstract module.
 * End-user of the TA-LIB shall never attempt to access these
 * structure directly.
 */

#ifndef TA_FRAME_PRIV_H
#define TA_FRAME_PRIV_H

#ifndef TA_ABSTRACT_H
   #include "ta_abstract.h"
#endif

#ifndef TA_MAGIC_NB_H
   #include "ta_magic_nb.h"
#endif

typedef struct
{
   const TA_Real      *open;
   const TA_Real      *high;
   const TA_Real      *low;
   const TA_Real      *close;
   const TA_Integer   *volume;
   const TA_Integer   *openInterest;
   const TA_Timestamp *timestamp;
} TA_PricePtrs;

typedef struct
{
   union TA_ParamHolderInputData
   {
      const TA_Real      *inReal;
      const TA_Integer   *inInteger;
      const TA_Draw      *inDraw;
      TA_PricePtrs        inPrice;
   } data;

   const TA_InputParameterInfo *inputInfo;

} TA_ParamHolderInput;

typedef struct
{
   union TA_ParamHolderOptInData
   {
      TA_Integer optInInteger;
      TA_Real    optInReal;
   } data;

   const TA_OptInputParameterInfo *optInputInfo;

} TA_ParamHolderOptInput;

typedef struct
{
   union TA_ParamHolderOutputData
   {
      TA_Real        *outReal;
      TA_Integer     *outInteger;
      TA_Draw        *outDraw;
   } data;

   const TA_OutputParameterInfo *outputInfo;
} TA_ParamHolderOutput;

typedef enum
{
   TA_PARAM_HOLDER_INPUT,
   TA_PARAM_HOLDER_OPTINPUT,
   TA_PARAM_HOLDER_OUTPUT
} TA_ParamHolderType;

typedef struct
{
   /* Magic number is used to detect internal error. */
   const unsigned int magicNumber;

   unsigned int valueInitialize; /* Boolean indicating that the user did called
                                  * the function to set the param holder.
                                  */

   TA_ParamHolderType type; /* Identify also which of the structure shall
                             * be used in the following 'p' union.
                             */
   union TA_ParamHolderValue
   {
      TA_ParamHolderInput    in;
      TA_ParamHolderOptInput optIn;
      TA_ParamHolderOutput   out;
   } p;

   /* These parameters shall be used only with this function. */
   const void *function;  /* At runtime, points on a TA_FrameFunction */
} TA_ParamHolderPriv;

typedef TA_RetCode (*TA_FrameFunction)( TA_Integer  startIdx,
                                        TA_Integer  endIdx,
                                        TA_Integer *outBegIdx,
                                        TA_Integer *outNbElement,
                                        TA_ParamHolderPriv *in,
                                        TA_ParamHolderPriv *optIn,
                                        TA_ParamHolderPriv *out );
#endif
