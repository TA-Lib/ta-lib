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

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;

import com.tictactec.ta.lib.Core;
import com.tictactec.ta.lib.MInteger;
import com.tictactec.ta.lib.RetCode;

/**
 * @deprecated in favor of CoreMetaData class
 */
public class TaFuncMetaInfo extends TaFuncSignature
{
   Method method;
   Method lookbackMethod;
   Class[] outVarTypes;
   Class[] optionTypes;
   Class[] parameterTypes;

   static final int TOTAL_FIX_PARAMETERS = 4;
   static final int INPUT_FIX_PARAMETERS = 2;
   static final int OUTPUT_FIX_PARAMETERS = 2;

   public TaFuncMetaInfo(String name, Method method, Method lookbackMethod)
   {
      this.name = name;doForEachFunctionPhase2
      this.method = method;
      this.lookbackMethod = lookbackMethod;
      this.optionTypes = lookbackMethod.getParameterTypes();
      this.parameterTypes = method.getParameterTypes();

      int ins = getNumberOfInputParameters();
      int outs = getNumberOfOutputParameters();

      createVarTypes(ins, outs);
   }

   int getNumberOfInputParameters()
   {
      return getFirstMInteger() - optionTypes.length - INPUT_FIX_PARAMETERS;
   }

   int getNumberOfOutputParameters()
   {
      return parameterTypes.length - getFirstMInteger() - OUTPUT_FIX_PARAMETERS;
   }

   int getFirstMInteger()
   {
      for (int i = 0; i < parameterTypes.length; i++)
      {
         if (parameterTypes[i].equals(MInteger.class))
         {
            return i;
         }
      }
      return -1;
   }

   void createVarTypes(int ins, int outs)
   {
      assert ins > 0 && outs > 0;
      inVarTypes = new Class[ins];
      outVarTypes = new Class[outs];

      for (int i = 0; i < ins; i++)
      {
         inVarTypes[i] = parameterTypes[INPUT_FIX_PARAMETERS + i];
      }
      int offset = INPUT_FIX_PARAMETERS + ins + optionTypes.length
            + OUTPUT_FIX_PARAMETERS;
      for (int i = 0; i < outs; i++)
      {
         outVarTypes[i] = parameterTypes[offset + i];
      }
   }

   public Class[] getInVarTypes()
   {
      return inVarTypes;
   }

   public Method getLookbackMethod()
   {
      return lookbackMethod;
   }

   public Method getMethod()
   {
      return method;
   }

   public String getName()
   {
      return name;
   }

   public Class[] getOptionTypes()
   {
      return optionTypes;
   }

   public Class[] getOutVarTypes()
   {
      return outVarTypes;
   }

   public boolean is11()
   {
      return inVarTypes.length == 1 && outVarTypes.length == 1;
   }

   public boolean is1N()
   {
      return inVarTypes.length == 1 && outVarTypes.length > 1;
   }

   public boolean isN1()
   {
      return inVarTypes.length > 1 && outVarTypes.length == 1;
   }

   public boolean isNN()
   {
      return inVarTypes.length > 1 && outVarTypes.length > 1;
   }

   public RetCode call(Core taCore, Object[] inArs, int startIndex, int endIndex, Object[] outArs, MInteger outBegIdx, MInteger outNbElement, Object ... options) throws IllegalArgumentException, IllegalAccessException, InvocationTargetException{
      Object[] parameters = new Object[inArs.length+outArs.length+options.length+TOTAL_FIX_PARAMETERS];
      parameters[0] = startIndex;
      parameters[1] = endIndex;
      int i=2;
      for(int j=0;j<inVarTypes.length;j++)
      {
         parameters[i++] = inArs[j];
      }
      for(Object opt : options){
         parameters[i++] = opt;
      }
      parameters[i++] = outBegIdx;
      parameters[i++] = outNbElement;
      for(int j=0;j<outVarTypes.length;j++)
      {
         parameters[i++] = outArs[j];
      }
      return (RetCode) getMethod().invoke(taCore, parameters);
   }

   public int callLookback(Core taCore, Object ... options) throws IllegalArgumentException, IllegalAccessException, InvocationTargetException
   {
      System.out.println(getLookbackMethod());
      return (Integer) getLookbackMethod().invoke(taCore, options);
   }

   public String toString()
   {
      StringBuffer sb = new StringBuffer();
      sb.append(name + "[");
      sb.append(inVarTypes.length);
      sb.append(":");
      sb.append(outVarTypes.length);
      sb.append("]");

      sb.append("IN:(");
      for (int i = 0; i < inVarTypes.length; i++)
      {
         sb.append(inVarTypes[i].getName());
         if (i < (inVarTypes.length - 1))
            sb.append(",");
      }
      sb.append(") OUT(");
      for (int i = 0; i < outVarTypes.length; i++)
      {
         sb.append(outVarTypes[i].getName());
         if (i < (outVarTypes.length - 1))
            sb.append(",");
      }
      sb.append(")");
      return sb.toString();
   }

}
