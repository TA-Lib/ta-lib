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
   } data;

   const TA_OutputParameterInfo *outputInfo;
} TA_ParamHolderOutput;

typedef struct
{
   /* Magic number is used to detect internal error. */
   unsigned int magicNumber;

   TA_ParamHolderInput    *in;
   TA_ParamHolderOptInput *optIn;
   TA_ParamHolderOutput   *out;

   /* Indicate which parameter have been initialized.
    * The LSB (Less Significant Bit) is the first parameter
    * and a bit equal to '1' indicate that the parameter is
    * not initialized.
    */
   unsigned int inBitmap;
   unsigned int outBitmap;

   const TA_FuncInfo *funcInfo;
} TA_ParamHolderPriv;

typedef TA_RetCode (*TA_FrameFunction)( const TA_ParamHolderPriv *params,
                                        TA_Integer  startIdx,
                                        TA_Integer  endIdx,
                                        TA_Integer *outBegIdx,
                                        TA_Integer *outNbElement );

typedef unsigned int (*TA_FrameLookback)( const TA_ParamHolderPriv *params );

#endif
