#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include <float.h>

#include "ta_utility.h"
#include "ta_func.h"

/* For handling the unstable period of some TA function. */
unsigned int TA_UnstablePeriodTable[TA_FUNC_UNST_ALL];

TA_RetCode TA_SetUnstablePeriod( TA_Libc      *libHandle,
                                 TA_FuncUnstId id,
                                 unsigned int  unstablePeriod )
{
   unsigned int i;

   (void)libHandle;

   if( id > TA_FUNC_UNST_ALL )
      return TA_BAD_PARAM;

   if( id == TA_FUNC_UNST_ALL )
   {
      for( i=0; i < TA_FUNC_UNST_ALL; i++ )
         TA_UnstablePeriodTable[i] = unstablePeriod;
   }
   else
   {
      TA_UnstablePeriodTable[id] = unstablePeriod;
   }

   return TA_SUCCESS;
}

unsigned int TA_GetUnstablePeriod( TA_Libc      *libHandle,
                                   TA_FuncUnstId id)
{
   (void)libHandle;

   if( id >= TA_FUNC_UNST_ALL )
      return TA_BAD_PARAM;

   return TA_UnstablePeriodTable[id];
}

