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

public class InputData
{
   String name;
   double[] doubleData;
   float[] floatData;
   int[] intData;
   
   public InputData(String name, int size)
   {
      this.name = name;
      this.doubleData = new double[size];
      this.floatData = new float[size];
      this.intData = new int[size];
   }

   /**
    * copy constructor to avoid clone()
    * @param that
    */
   public InputData(InputData that)
   {
      this(that.name, that.size());
      System.arraycopy(that.doubleData,0,this.doubleData,0,this.doubleData.length);
      System.arraycopy(that.floatData,0,this.floatData,0,this.floatData.length);
      System.arraycopy(that.intData,0,this.intData,0,this.intData.length);
   }

   public String getName()
   {
      return name;
   }
   
   public int size()
   {
      return doubleData.length;
   }

   public void setData(int index, double dd, float fd, int id)
   {
      doubleData[index] = dd;
      floatData[index] = fd;
      intData[index] = id;
   }

   public double[] getDoubleData()
   {
      return doubleData;
   }

   public float[] getFloatData()
   {
      return floatData;
   }

   public int[] getIntData()
   {
      return intData;
   }

}
