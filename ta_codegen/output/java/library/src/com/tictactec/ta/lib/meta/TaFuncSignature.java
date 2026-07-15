/* TA-LIB Copyright (c) 1999-2007, Mario Fortier
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
 *  BT       Barry Tsung
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  022206 BT     First Version
 *  031107 RG     Deprecated
 */

package com.tictactec.ta.lib.meta;

/**
 * @deprecated in favor of CoreMetaData
 */
public class TaFuncSignature implements Comparable
{
   String name;
   Class[] inVarTypes;

   public TaFuncSignature()
   {
   }

   public TaFuncSignature(String name, Class[] inVarTypes)
   {
      this.name = name;
      this.inVarTypes = inVarTypes;
   }

   final public int compareTo(Object rhs)
   {
      int rtn;
      TaFuncSignature that = (TaFuncSignature) rhs;
      if ((rtn = name.compareTo(that.name)) != 0)
      {
         return rtn;
      } else
      {
         if (inVarTypes.length != that.inVarTypes.length)
         {
            return inVarTypes.length - that.inVarTypes.length;
         }
         for (int i = 0; i < inVarTypes.length; i++)
         {
            if ((rtn = inVarTypes[i].getName().compareTo(
                  that.inVarTypes[i].getName())) != 0)
            {
               return rtn;
            }
         }
      }
      return 0;
   }

   public Class[] getInVarTypes()
   {
      return inVarTypes;
   }

   public String getName()
   {
      return name;
   }

}
