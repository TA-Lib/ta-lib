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
 *  031107 RG     Deprecating this class in favor of CoreMetaData
 */

package com.tictactec.ta.lib.meta;

import java.lang.reflect.Method;
import java.util.Collection;
import java.util.HashMap;
import java.util.Map;
import java.util.TreeMap;

import com.tictactec.ta.lib.Core;
import com.tictactec.ta.lib.RetCode;

/**
 * @deprecated in favor of CoreMetaData class
 */
public class CoreMetaInfo
{
   static final Class coreClass = Core.class;

   static final String LOOKBACK_SUFFIX = "_Lookback";

   static final String INT_PREFIX = "INT_";

   Map<TaFuncSignature, TaFuncMetaInfo> taFuncMap;

   public CoreMetaInfo()
   {
      this.taFuncMap = getTaFuncMetaInfoMap();
   }

   protected Map<String, Method> getLookbackMethodMap()
   {
      Map<String, Method> map = new HashMap<String, Method>();
      Method[] ms = coreClass.getDeclaredMethods();
      for (Method m : ms)
      {
         if (m.getName().endsWith(LOOKBACK_SUFFIX))
         {
            map.put(m.getName(), m);
            // System.out.println("lookback="+m.getName());
         }
      }
      return map;
   }

   @SuppressWarnings("unchecked")
   protected Map<TaFuncSignature, TaFuncMetaInfo> getTaFuncMetaInfoMap()
   {
      Map<TaFuncSignature, TaFuncMetaInfo> result = new TreeMap<TaFuncSignature, TaFuncMetaInfo>();
      Method[] ms = coreClass.getDeclaredMethods();
      Map<String, Method> lookbackMap = getLookbackMethodMap();
      for (Method taMethod : ms)
      {
         String fn = taMethod.getName();
         if (taMethod.getReturnType().equals(RetCode.class)
               && !fn.startsWith(INT_PREFIX))
         {
            String lookbackName = fn + LOOKBACK_SUFFIX;
            Method lookbackMethod = lookbackMap.get(lookbackName);
            if (lookbackMethod != null)
            {
               // System.out.println(fn+","+lookbackName);
               TaFuncMetaInfo mi = new TaFuncMetaInfo(fn, taMethod,
                     lookbackMethod);
               result.put(mi, mi);
            }
         }
      }
      return result;
   }

   public Collection<TaFuncMetaInfo> getAllFuncs()
   {
      return taFuncMap.values();
   }

   public TaFuncMetaInfo get(String taName, Class[] inVarTypes)
   {
      return taFuncMap.get(new TaFuncSignature(taName, inVarTypes));
   }

   public void forEach(TaFuncClosure closure) throws Exception
   {
      for (TaFuncMetaInfo mi : getAllFuncs())
      {
         closure.execute(mi);
      }
   }

   public static void main(String[] args)
   {
      CoreMetaInfo mi = new CoreMetaInfo();
      Collection<TaFuncMetaInfo> fs = mi.getAllFuncs();
      int i = 0;
      for (TaFuncMetaInfo f : fs)
      {
         System.out.println(" " + (i++) + " " + f);
      }
   }
}
