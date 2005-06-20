/* TA-LIB Copyright (c) 1999-2004, Mario Fortier
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in
 *   the documentation and/or other materials provided with the
 *   distribution.
 *
 * - Neither name of author nor the names of its contributors
 *   may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *  RM       Robert Meier (talib@meierlim.com http://www.meierlim.com)
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  052603 MF     Adapt code to compile with .NET Managed C++
 *  123004 RM,MF  Adapt code to work with Visual Studio 2005
 *
 */

#if defined( _MANAGED )
   #using <mscorlib.dll>
   #include "TA-Lib-Core.h"
   namespace TA { namespace Lib {
#else
   #include "ta_utility.h"
   #include "ta_func.h"
#endif

#if defined( _MANAGED )
 enum class Core::TA_RetCode Core::SetUnstablePeriod(  enum class TA_FuncUnstId id,
                                                       unsigned int  unstablePeriod )
#else
TA_RetCode TA_SetUnstablePeriod( TA_FuncUnstId id,
                                 unsigned int  unstablePeriod )
#endif
{
   unsigned int i;

   if( id > NAMESPACE(TA_FuncUnstId)TA_FUNC_UNST_ALL )
      return NAMESPACE(TA_RetCode)TA_BAD_PARAM;

   if( id == NAMESPACE(TA_FuncUnstId)TA_FUNC_UNST_ALL )
   {
      for( i=0; i < (int)NAMESPACE(TA_FuncUnstId)TA_FUNC_UNST_ALL; i++ )
         TA_Globals->unstablePeriod[i] = unstablePeriod;
   }
   else
   {
      TA_Globals->unstablePeriod[(int)id] = unstablePeriod;
   }

   return NAMESPACE(TA_RetCode)TA_SUCCESS;
}

#if defined( _MANAGED )
unsigned int Core::GetUnstablePeriod( enum class TA_FuncUnstId id )
#else
unsigned int TA_GetUnstablePeriod( TA_FuncUnstId id )
#endif
{
   if( id >= NAMESPACE(TA_FuncUnstId)TA_FUNC_UNST_ALL )
      return 0;

   return TA_Globals->unstablePeriod[(int)id];
}

#if defined( _MANAGED )
 enum class Core::TA_RetCode Core::SetCompatibility(  enum class TA_Compatibility value )
#else
TA_RetCode TA_SetCompatibility( TA_Compatibility value )
#endif
{
   TA_Globals->compatibility = value;
   return NAMESPACE(TA_RetCode)TA_SUCCESS;
}

#if defined( _MANAGED )
 enum class Core::TA_Compatibility Core::GetCompatibility( void )
#else
TA_Compatibility TA_GetCompatibility( void )
#endif
{
   return TA_Globals->compatibility;
}

#if defined( _MANAGED )
}} // Close namespace TA.Lib
#endif
