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

public class TestData
{
   List<InputData> allInputData;
   InputData inputNegData; // new double[100];

   InputData inputZeroData; // new double[100];

   InputData inputRandFltEpsilon; // new double[100];

   InputData inputRandDblEpsilon; // new double[100];

   InputData inputRandomData; // new double[2000];

   final static double output[][] = new double[10][2000];

   final static int output_int[][] = new int[10][2000];

   public final static double DBL_EPSILON = 2.2204460492503131e-016;

   public final static double FLT_EPSILON = 1.192092896e-07;

   /* min/max value for a TA_Integer */
   public final static int TA_INTEGER_MIN = Integer.MIN_VALUE + 1;
   public final static int TA_INTEGER_MAX = Integer.MAX_VALUE;

   /* min/max value for a TA_Real 
    *
    * Use fix value making sense in the
    * context of TA-Lib (avoid to use DBL_MIN
    * and DBL_MAX standard macro because they
    * are troublesome with some compiler).
    */
   public final static double TA_REAL_MIN = (-3e+37);
   public final static double TA_REAL_MAX = (3e+37);

   /* A value outside of the min/max range 
    * indicates an undefined or default value.
    */
   public final static double TA_INTEGER_DEFAULT = Integer.MIN_VALUE;
   public final static double TA_REAL_DEFAULT = (-4e+37);

   TestData()
   {
      allInputData = new ArrayList<InputData>();
      inputNegData = new InputData("Negative Data", 100);
      allInputData.add(inputNegData);
      inputZeroData = new InputData("Zero Data", 100);
      allInputData.add(inputZeroData);
      inputRandFltEpsilon = new InputData("Random Float Epsilon", 100);
      allInputData.add(inputRandFltEpsilon);
      inputRandDblEpsilon = new InputData("Random Double Epsilon", 100);
      allInputData.add(inputRandDblEpsilon);
      inputRandomData = new InputData("Random Data", 2000);
      allInputData.add(inputRandomData);

      for (int i = 0; i < inputNegData.size(); i++)
      {
         inputNegData.setData(i, -((double) i), -((float) i), -i);
      }

      for (int i = 0; i < inputZeroData.size(); i++)
      {
         inputZeroData.setData(i, 0.0, (float) 0.0, 0);
      }

      for (int i = 0; i < inputRandomData.size(); i++)
      {
         double rand = Math.random() / 97.234;
         inputRandomData.setData(i, rand, (float) rand, (int) rand);
      }

      for (int i = 0; i < inputRandFltEpsilon.size(); i++)
      {
         int sign = ((int) Math.random()) % 2;
         double data = (sign != 0 ? 1.0 : -1.0) * (FLT_EPSILON);
         inputRandFltEpsilon.setData(i, data, (float) data,
               sign != 0 ? TA_INTEGER_MIN : TA_INTEGER_MAX);
      }

      for (int i = 0; i < inputRandDblEpsilon.size(); i++)
      {
         int sign = ((int) Math.random()) % 2;
         double data = (sign != 0 ? 1.0 : -1.0) * (DBL_EPSILON);
         inputRandDblEpsilon.setData(i, data, (float) data, sign != 0 ? 1 : -1);
      }
   }
   
   public List<InputData> getAllInputData()
   {
      return allInputData;
   }
}
