#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include <float.h>

#include "ta_utility.h"
#include "ta_func.h"

TA_RetCode TA_SetUnstablePeriod( TA_FuncUnstId id,
                                 unsigned int  unstablePeriod )
{
   unsigned int i;

   if( id > TA_FUNC_UNST_ALL )
      return TA_BAD_PARAM;

   if( id == TA_FUNC_UNST_ALL )
   {
      for( i=0; i < TA_FUNC_UNST_ALL; i++ )
         TA_Globals.unstablePeriod[i] = unstablePeriod;
   }
   else
   {
      TA_Globals.unstablePeriod[id] = unstablePeriod;
   }

   return TA_SUCCESS;
}

unsigned int TA_GetUnstablePeriod( TA_FuncUnstId id )
{
   if( id >= TA_FUNC_UNST_ALL )
      return TA_BAD_PARAM;

   return TA_Globals.unstablePeriod[id];
}


TA_RetCode TA_SetCompatibility( TA_Compatibility value )

{
   TA_Globals.compatibility = value;
   return TA_SUCCESS;
}

TA_Compatibility TA_GetCompatibility( void )
{
   return TA_Globals.compatibility;
}
