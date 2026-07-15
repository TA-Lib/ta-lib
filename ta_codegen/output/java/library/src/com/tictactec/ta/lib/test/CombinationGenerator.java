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
 */

package com.tictactec.ta.lib.test;

import java.util.ArrayList;
import java.util.List;

public class CombinationGenerator
{  
   public CombinationGenerator()
   {
   }
   
   public List<Object[]> getAllCombinations(Object[] parms)
   { 
      List<Object[]> children=null;
      if( parms.length > 1 ){
         Object[] cp = new Object[parms.length-1];
         for(int i=0;i<parms.length-1;i++){
            cp[i] = parms[i+1];
         }
         children = getAllCombinations(cp);
      }
      List<Object[]> ret = new ArrayList<Object[]>();
      if( parms.length == 0 ){
         ret.add(parms);
      }else{
         if( parms[0].getClass().isArray() ){
            for(Object o : (Object[]) parms[0]){
               process(ret, children, o);
            }
         }else{
            process(ret, children, parms[0]);
         }
      }
      return ret;
   }

   void process(List<Object[]> ret, List<Object[]> children, Object obj)
   {
      if( children == null ){
         Object[] da = new Object[1];
         da[0] = obj;
         ret.add(da);
      }else{
         for(Object[] cr : children){
            Object[] rr = new Object[cr.length+1];
            rr[0] = obj;
            for(int i=0;i<cr.length;i++){
               rr[i+1] = cr[i];
            }
            ret.add(rr);
         }
      }
   }
}
