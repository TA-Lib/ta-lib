#if defined( _MANAGED )
   #using <mscorlib.dll>
   #include "Core.h"
   namespace TA { namespace Lib {
#else
   #include "ta_utility.h"
   #include "ta_func.h"
#endif

#if defined( _MANAGED )
enum TA_RetCode Core::SetUnstablePeriod( enum TA_FuncUnstId id,
                                         unsigned int  unstablePeriod )
#else
TA_RetCode TA_SetUnstablePeriod( TA_FuncUnstId id,
                                 unsigned int  unstablePeriod )
#endif
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

#if defined( _MANAGED )
unsigned int Core::GetUnstablePeriod( enum TA_FuncUnstId id )
#else
unsigned int TA_GetUnstablePeriod( TA_FuncUnstId id )
#endif
{
   if( id >= TA_FUNC_UNST_ALL )
      return TA_BAD_PARAM;

   return TA_Globals.unstablePeriod[id];
}

#if defined( _MANAGED )
enum TA_RetCode Core::SetCompatibility( enum TA_Compatibility value )
#else
TA_RetCode TA_SetCompatibility( TA_Compatibility value )
#endif
{
   TA_Globals.compatibility = value;
   return TA_SUCCESS;
}

#if defined( _MANAGED )
enum TA_Compatibility Core::GetCompatibility( void )
#else
TA_Compatibility TA_GetCompatibility( void )
#endif
{
   return (TA_Compatibility)TA_Globals.compatibility;
}

#if defined( _MANAGED )
}} // Close namespace TA.Lib
#endif
