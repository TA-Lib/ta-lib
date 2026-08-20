/* TA-LIB Copyright (c) 1999-2026, Mario Fortier
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
 *  072726 MF,CC  Bound TA_Set/GetUnstablePeriod below as well as above (#144)
 *  072826 MF,CC  Range-check against TA_FUNC_UNST_COUNT; ALL is now INT_MAX
 *
 */

#include "ta_utility.h"
#include "ta_func.h"
#include "ta_memory.h"

TA_RetCode TA_SetUnstablePeriod( TA_FuncUnstId id,
                                 unsigned int  unstablePeriod )
{
   int i;

   /* The wildcard is INT_MAX, far above every id, so it is tested by value and
    * everything else must land inside the table. The unsigned compare wraps a
    * negative id past the count instead of indexing behind the array -- an
    * out-of-bounds write onto the adjacent TA_Globals->compatibility (#144) --
    * and stays correct whether the compiler gives the enum a signed or unsigned
    * underlying type (any width up to unsigned int).
    */
   if( id != TA_FUNC_UNST_ALL &&
       (unsigned int)id >= (unsigned int)TA_FUNC_UNST_COUNT )
      return TA_BAD_PARAM;

   /* The period is added to a lookback which is then used as an index, so an
    * unbounded one overflows that lookback NEGATIVE and the function indexes
    * far past the end of its input. TA_MAX_INDEX is the ceiling the index space
    * already enforces on startIdx/endIdx; a warm-up longer than the largest
    * addressable series could never produce output, so nothing legitimate is
    * refused. Guarding here rather than in each lookback keeps the invariant in
    * one place -- every unstable-period function derives its lookback from this
    * value.
    */
   if( unstablePeriod > (unsigned int)TA_MAX_INDEX )
      return TA_BAD_PARAM;

   if( id == TA_FUNC_UNST_ALL )
   {
      for( i=0; i < TA_FUNC_UNST_COUNT; i++ )
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
   /* Unsigned compare -- see TA_SetUnstablePeriod above. The wildcard names no
    * single function, so it and any out-of-range id read as 0 rather than off
    * the end of the array.
    */
   if( (unsigned int)id >= (unsigned int)TA_FUNC_UNST_COUNT )
	   return 0;

   return TA_Globals->unstablePeriod[id];
}

TA_RetCode TA_SetCompatibility( TA_Compatibility value )
{
   /* Reject a value outside the enum rather than latching it. Without this the
    * setter accepted anything and the getter echoed it back, so a caller had no
    * way to tell a typo from a setting (open item 10 of
    * docs/error-handling-spec.md). The function is deprecated; this is the whole
    * fix, not a step toward a larger one.
    */
   if( value != TA_COMPATIBILITY_DEFAULT && value != TA_COMPATIBILITY_METASTOCK )
      return TA_BAD_PARAM;

   TA_GLOBALS_COMPATIBILITY = value;
   return TA_SUCCESS;
}

TA_Compatibility TA_GetCompatibility( void )
{
   return TA_GLOBALS_COMPATIBILITY;
}
