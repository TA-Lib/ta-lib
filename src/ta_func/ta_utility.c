/* TA-LIB Copyright (c) 1999-2025, Mario Fortier
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
 *  071926 MF,CC  Remove dead .NET/Java preprocessor branches (plain C only)
 *
 */

#include "ta_utility.h"
#include "ta_func.h"
#include "ta_memory.h"

TA_RetCode TA_SetUnstablePeriod( TA_FuncUnstId id,
                                 unsigned int  unstablePeriod )
{
   int i;

   if( id > TA_FUNC_UNST_ALL )
      return TA_BAD_PARAM;

   if( id == TA_FUNC_UNST_ALL )
   {
      for( i=0; i < (int)TA_FUNC_UNST_ALL; i++ )
	  {
         TA_Globals->unstablePeriod[i] = unstablePeriod;
	  }
   }
   else
   {
      TA_Globals->unstablePeriod[id] = unstablePeriod;
   }

   return TA_SUCCESS;
}

unsigned int TA_GetUnstablePeriod( TA_FuncUnstId id )
{
   if( id >= TA_FUNC_UNST_ALL )
	   return 0;

   return TA_Globals->unstablePeriod[id];
}

TA_RetCode TA_SetCompatibility( TA_Compatibility value )
{
   TA_GLOBALS_COMPATIBILITY = value;
   return TA_SUCCESS;
}

TA_Compatibility TA_GetCompatibility( void )
{
   return TA_GLOBALS_COMPATIBILITY;
}
