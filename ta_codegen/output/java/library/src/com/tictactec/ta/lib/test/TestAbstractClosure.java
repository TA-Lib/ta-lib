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

import java.lang.reflect.Field;
import java.util.ArrayList;
import java.util.List;
import java.util.Arrays;


import com.tictactec.ta.lib.Core;
import com.tictactec.ta.lib.MInteger;
import com.tictactec.ta.lib.RetCode;
import com.tictactec.ta.lib.meta.TaFuncClosure;
import com.tictactec.ta.lib.meta.TaFuncMetaInfo;

public class TestAbstractClosure extends junit.framework.Assert implements TaFuncClosure
{
   List<InputData> originalData;  // original input data to detect any overwritten.
   List<InputData> testData;      // clone of originalData
   Core taCore;
   InputData curInputData;
   
   public TestAbstractClosure(Core taCore, List<InputData> data)
   {
      this.taCore = taCore;
      this.originalData = data;
      this.testData = new ArrayList<InputData>();
      // clone originalData
      for(InputData id : originalData)
      {
         testData.add(new InputData(id));
      }
   }

   public void execute(TaFuncMetaInfo mi) throws Exception
   {
      int n=testData.size();
      for(int i=0;i<n;i++)
      {
         execute(mi, testData.get(i), originalData.get(i));
      }
   }
   
   void execute(TaFuncMetaInfo mi, InputData inputData, InputData originalInputData) throws Exception
   {
      curInputData = inputData;
      // load input, output parameters and options.
      Object[] inArs   = getInputParamters(mi, inputData);
      Object[] options = getDefaultOptions(mi);
      
      // test all possible Enum combinations. 
      List<Object[]> optionsCombination = new CombinationGenerator().getAllCombinations(options);
      for(Object[] ops: optionsCombination){
         execute(mi, inputData, originalInputData, inArs, ops);
      }
   }
      
   void execute(TaFuncMetaInfo mi, InputData inputData, InputData originalInputData, Object[] inArs, Object[] options) throws Exception
   {
      int lookback = 0; //mi.callLookback(taCore, options);
      Object[] outArs  = getOutputParameters(mi, inputData.size(), lookback);
      MInteger outBegIdx = new MInteger();
      MInteger outNbElement = new MInteger();
      int startIndex = 0;
      int endIndex = inputData.size()-1;
      
      /* Do the function call. */
      RetCode retCode = mi.call(taCore, inArs, startIndex, endIndex, outArs, outBegIdx, outNbElement, options);
      assertEquals(retCode, RetCode.Success);
      
      /* Verify consistency with Lookback */
/*      
      if( lookback != outBegIdx.value ){
         // problem
         assert false;
      }
*/      
      // verify output data
      assert(verifyOutputData(mi, outArs));

      // verify input data to detect any overwritten situtaion.
      assert(verifyInputData(inputData, originalInputData));

      /* Do another function call where startIdx == endIdx == 0.
       * In that case, outBegIdx should ALWAYS be zero.
       */
      retCode = mi.call(taCore, inArs, 0, 0, outArs, outBegIdx, outNbElement, options);
      assertEquals(retCode, RetCode.Success );
      assertEquals(outBegIdx.value, 0);
   }
   
   Object[] getInputParamters(TaFuncMetaInfo mi, InputData inputData)
   {
      Class[] inVarTypes = mi.getInVarTypes();
      Object[] ret = new Object[inVarTypes.length];
      for(int i=0;i<inVarTypes.length;i++)
      {
         if( inVarTypes[i].equals(double[].class) )
         {
            ret[i] = inputData.getDoubleData();
         }
         else if( inVarTypes[i].equals(float[].class) )
         {
            ret[i] = inputData.getFloatData();
         }
         else if( inVarTypes[i].equals(int[].class) )
         {
            ret[i] = inputData.getIntData();
         }
         else
         {
            fail("Invalid input type : "+inVarTypes[i]);
         }
      }
      return ret;
   }

   Object[] getOutputParameters(TaFuncMetaInfo mi, int inSize, int lookback)
   {
      int outSize=inSize;
      Class[] outVarTypes = mi.getOutVarTypes();
      Object[] ret = new Object[outVarTypes.length];
      for(int i=0;i<outVarTypes.length;i++)
      {
         if( outVarTypes[i].equals(double[].class) )
         {
//            System.out.println("outSize="+outSize+",inSize="+inSize+",lookback="+lookback);
            ret[i] = new double[outSize];
            Arrays.fill((double[])ret[i],TestData.TA_REAL_MIN);
         }
         else if( outVarTypes[i].equals(float[].class) )
         {
            ret[i] = new float[outSize];
            Arrays.fill((float[])ret[i],(float)TestData.TA_REAL_MIN);
         }
         else if( outVarTypes[i].equals(int[].class) )
         {
            ret[i] = new int[outSize];
            Arrays.fill((int[])ret[i],TestData.TA_INTEGER_MIN);
         }
         else
         {
            fail("Invalid output type : "+outVarTypes[i]);
         }
      }
      return ret;      
   }
   
   Object[] getDefaultOptions(TaFuncMetaInfo mi) throws IllegalArgumentException, IllegalAccessException
   {
      Class[] optionTypes = mi.getOptionTypes();
      Object[] ret = new Object[optionTypes.length];
      for(int i=0;i<optionTypes.length;i++){
         if( optionTypes[i].equals(double.class) )
         {
            ret[i] = TestData.TA_REAL_DEFAULT;
         }
         else if( optionTypes[i].equals(float.class) )
         {
            ret[i] = TestData.TA_REAL_DEFAULT;
         }
         else if( optionTypes[i].equals(int.class) )
         {
            ret[i] = Integer.MIN_VALUE;
         }
         else if( optionTypes[i].isEnum() )
         {
            // return all the members from this Enum.
            ret[i] = getAllEnumMembers(optionTypes[i]);
         }
         else
         {
            fail("Invalid option type : "+optionTypes[i]);
         }
      }
      return ret;
   }
   
   Enum[] getAllEnumMembers(Class clazz) throws IllegalArgumentException, IllegalAccessException
   {
      Field[] flds = clazz.getFields();      
      Enum[] ret = new Enum[flds.length];
      for(int i=0;i<flds.length;i++){
         ret[i] = (Enum) flds[i].get(clazz);
      }
      return ret;
   }
   
   boolean verifyOutputData(TaFuncMetaInfo mi, Object[] outArs)
   {
      Class[] outVarTypes = mi.getOutVarTypes();
      for(int i=0;i<outVarTypes.length;i++)
      {
         if( outVarTypes[i].equals(double[].class) )
         {
            double[] ar = (double[]) outArs[i];
            int j=0;
            for(double d:ar){
               if( Double.isNaN(d) || Double.isInfinite(d) ){
                  // problem
                  System.out.println("----------->"+mi.toString()+"["+j+"]="+d+":"+curInputData.getName());
                  //return false;
               }
               j++;
            }
         }
         else if( outVarTypes[i].equals(float[].class) )
         {
         }
         else if( outVarTypes[i].equals(int[].class) )
         {
         }
         else
         {
            fail("invalid output type : "+outVarTypes[i]);
         }
      }
      return true;
   }
   
   boolean verifyInputData(InputData inputData, InputData originalInputData)
   {
      return
         Arrays.equals(inputData.getDoubleData(),originalInputData.getDoubleData()) &&
         Arrays.equals(inputData.getFloatData(),originalInputData.getFloatData())  &&
         Arrays.equals(inputData.getIntData(),originalInputData.getIntData());
   }
}
