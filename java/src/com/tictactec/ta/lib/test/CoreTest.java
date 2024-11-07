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
 *  MF       Mario Fortier
 *
 * Change history:
 *
 *  MMDDYY BY     Description
 *  -------------------------------------------------------------------
 *  121005 MF     First Version
 */

package com.tictactec.ta.lib.test;

import junit.framework.*;
import java.util.Arrays;
import com.tictactec.ta.lib.*;

public class CoreTest extends TestCase {	
    private double input[];
    private int inputInt[];
    private double output[];
    private int outputInt[];
    private MInteger outBegIdx; 
    private MInteger outNbElement;
    private RetCode retCode;
    private Core lib;
    private int lookback;

    static public double[] close = new double[] {91.500000,94.815000,94.375000,95.095000,93.780000,94.625000,92.530000,92.750000,90.315000,92.470000,96.125000,
    	97.250000,98.500000,89.875000,91.000000,92.815000,89.155000,89.345000,91.625000,89.875000,88.375000,
    	87.625000,84.780000,83.000000,83.500000,81.375000,84.440000,89.250000,86.375000,86.250000,85.250000,
    	87.125000,85.815000,88.970000,88.470000,86.875000,86.815000,84.875000,84.190000,83.875000,83.375000,
    	85.500000,89.190000,89.440000,91.095000,90.750000,91.440000,89.000000,91.000000,90.500000,89.030000,
    	88.815000,84.280000,83.500000,82.690000,84.750000,85.655000,86.190000,88.940000,89.280000,88.625000,
    	88.500000,91.970000,91.500000,93.250000,93.500000,93.155000,91.720000,90.000000,89.690000,88.875000,
    	85.190000,83.375000,84.875000,85.940000,97.250000,99.875000,104.940000,106.000000,102.500000,102.405000,
    	104.595000,106.125000,106.000000,106.065000,104.625000,108.625000,109.315000,110.500000,112.750000,123.000000,
    	119.625000,118.750000,119.250000,117.940000,116.440000,115.190000,111.875000,110.595000,118.125000,116.000000,
    	116.000000,112.000000,113.750000,112.940000,116.000000,120.500000,116.620000,117.000000,115.250000,114.310000,
    	115.500000,115.870000,120.690000,120.190000,120.750000,124.750000,123.370000,122.940000,122.560000,123.120000,
    	122.560000,124.620000,129.250000,131.000000,132.250000,131.000000,132.810000,134.000000,137.380000,137.810000,
    	137.880000,137.250000,136.310000,136.250000,134.630000,128.250000,129.000000,123.870000,124.810000,123.000000,
    	126.250000,128.380000,125.370000,125.690000,122.250000,119.370000,118.500000,123.190000,123.500000,122.190000,
    	119.310000,123.310000,121.120000,123.370000,127.370000,128.500000,123.870000,122.940000,121.750000,124.440000,
    	122.000000,122.370000,122.940000,124.000000,123.190000,124.560000,127.250000,125.870000,128.860000,132.000000,
    	130.750000,134.750000,135.000000,132.380000,133.310000,131.940000,130.000000,125.370000,130.130000,127.120000,
    	125.190000,122.000000,125.000000,123.000000,123.500000,120.060000,121.000000,117.750000,119.870000,122.000000,
    	119.190000,116.370000,113.500000,114.250000,110.000000,105.060000,107.000000,107.870000,107.000000,107.120000,
    	107.000000,91.000000,93.940000,93.870000,95.500000,93.000000,94.940000,98.250000,96.750000,94.810000,
    	94.370000,91.560000,90.250000,93.940000,93.620000,97.000000,95.000000,95.870000,94.060000,94.620000,
    	93.750000,98.000000,103.940000,107.870000,106.060000,104.500000,105.000000,104.190000,103.060000,103.420000,
    	105.270000,111.870000,116.000000,116.620000,118.280000,113.370000,109.000000,109.700000,109.250000,107.000000,
    	109.190000,110.000000,109.200000,110.120000,108.000000,108.620000,109.750000,109.810000,109.000000,108.750000,
    	107.870000};
    
    public void test_MACD()
    {        
    	double macd[]   = new double[close.length];
    	double signal[] = new double[close.length];
    	double hist[]   = new double[close.length];
   	    lookback = lib.macdLookback(15,26,9);
        retCode = lib.macd(0,close.length-1,close,15,26,9,outBegIdx,outNbElement,macd,signal,hist);
        
        double ema15[] = new double[close.length];
   	    lookback = lib.emaLookback(15);
        retCode = lib.ema(0,close.length-1,close,15,outBegIdx,outNbElement,ema15);
        
        double ema26[] = new double[close.length];
   	    lookback = lib.emaLookback(26);
        retCode = lib.ema(0,close.length-1,close,26,outBegIdx,outNbElement,ema26);
        
        // TODO Add tests of outputs
    }
    
    public CoreTest(String testName) {
        super(testName);
        // Create the library (typically done only once).
        lib = new Core();
        input = new double[200];
        inputInt = new int[200];        
        output = new double[200];
        outputInt = new int[200];
        outBegIdx = new MInteger();
        outNbElement = new MInteger();
    }

    protected void setUp() {
    	for( int i=0; i < input.length; i++ )
    	{
    		input[i] = (double)i;
    		inputInt[i] = i;
    	}
    	for( int i=0; i < output.length; i++ )
    	{
    		output[i] = (double)-999999.0;
    		outputInt[i] = -999999;
    	}
    	outBegIdx.value = -1;
    	outNbElement.value = -1;
    	retCode = RetCode.InternalError;
    	lookback = -1;
    }

    protected void tearDown() 
    {
    	assertEquals(retCode.toString(),RetCode.Success.toString());
    	assertEquals(lookback,outBegIdx.value);
    }

    public static Test suite() {
        TestSuite suite = new TestSuite(CoreTest.class);
        
        return suite;
    }

    public void testMFI()
    {
    	 lookback = lib.mfiLookback(2);
         retCode = lib.mfi(0,input.length-1,input,input,input,input,2,outBegIdx,outNbElement,output);
    }
    
    public void testHT()
    {    	
    	lookback = lib.htTrendModeLookback();
    	retCode = lib.htTrendMode(0,input.length-1,input,outBegIdx,outNbElement,outputInt);
    }
   
    public void testMA_MAMA()
    {
        lookback = lib.movingAverageLookback(10,MAType.Mama);
        retCode = lib.movingAverage(0,input.length-1,input,10,MAType.Mama,outBegIdx,outNbElement,output);        
    }
    
    public void testMA_SMA()
    {
        lookback = lib.movingAverageLookback(10,MAType.Sma);
        retCode = lib.movingAverage(0,input.length-1,input,10,MAType.Sma,outBegIdx,outNbElement,output);
        assertEquals(outBegIdx.value,9);
    }
    
    public void testCMO()
    {
    	lookback = lib.cmoLookback(10);
    	retCode = lib.cmo(0,input.length-1,input,10,outBegIdx,outNbElement,output);
    	assertEquals(100.0,output[0]);
    }
    
    public void testSimpleCall() {        
                
        // Create Input/Output arrays.
        input[0] = 2.0;
        input[1] = 1.2;
        input[2] = 1.5;
                
        // Do the TA function call
        retCode = lib.max( 0, 2, input, 2,
                           outBegIdx, outNbElement,
                           output );
        
        // Test the results.
        assertEquals( retCode, RetCode.Success);
        assertEquals( outBegIdx.value, 1 );
        assertEquals( outNbElement.value, 2 );
        assertEquals( output[0], 2.0 );
        assertEquals( output[1], 1.5 );
        
        lookback = lib.maxLookback(2);
    }  
    
       public final static double FLT_EPSILON = 1.192092896e-07;
       public final static double TA_REAL_MIN = (-3e+37);
       
       public void testCMO2()
       {          
          // initialize inputRandFltEpsilon
          double[] inputRandFltEpsilon = new double[100];
          for (int i = 0; i < inputRandFltEpsilon.length; i++)
          {
             int sign = ((int) Math.random()) % 2;
             double data = (sign != 0 ? 1.0 : -1.0) * (FLT_EPSILON);
             inputRandFltEpsilon[i]= data;
          }
          // set default integer input option
          int optInTimePeriod = Integer.MIN_VALUE;
          
          // set output buffer
          double[] output = new double[100];
          Arrays.fill(output, TA_REAL_MIN);
          
          MInteger outBegIdx = new MInteger();
          MInteger outNbElement = new MInteger();
          
          int lookback = lib.cmoLookback(optInTimePeriod);
          retCode = lib.cmo(0,inputRandFltEpsilon.length-1,inputRandFltEpsilon,optInTimePeriod,outBegIdx,outNbElement,output);
          assertEquals( lookback, outBegIdx.value );
          assertEquals( output[0], 0.0 );
          assertEquals( output[1], 0.0 );
          assertEquals( output[85], 0.0 );
          assertEquals( output[86], TA_REAL_MIN );
          /*
          System.out.println("outBegIdx="+outBegIdx.value+",outNbElement="+outNbElement.value);

          for(int i=0;i<output.length;i++)
          {
             System.out.println("["+i+"]="+output[i]);
          }*/
       }

}
