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
 *  RG       Richard Gomes
 *
 * Change history:
 *
 *  YYYYMMDD BY     Description
 *  -------------------------------------------------------------------
 *  20070311 RG     First Version
 */

package com.tictactec.ta.lib.meta.helpers;

import java.lang.reflect.InvocationTargetException;
import java.util.List;

import com.tictactec.ta.lib.MInteger;
import com.tictactec.ta.lib.meta.CoreMetaData;
import com.tictactec.ta.lib.meta.annotation.FuncInfo;
import com.tictactec.ta.lib.meta.annotation.InputParameterInfo;
import com.tictactec.ta.lib.meta.annotation.InputParameterType;
import com.tictactec.ta.lib.meta.annotation.IntegerList;
import com.tictactec.ta.lib.meta.annotation.IntegerRange;
import com.tictactec.ta.lib.meta.annotation.OptInputParameterInfo;
import com.tictactec.ta.lib.meta.annotation.OutputParameterInfo;
import com.tictactec.ta.lib.meta.annotation.OutputParameterType;
import com.tictactec.ta.lib.meta.annotation.RealList;
import com.tictactec.ta.lib.meta.annotation.RealRange;


/**
 * This is a simple API level helper class based on CoreMetaData.
 * 
 * <p>This class provides the very simple functionality of calling dinamically a TA function once you already know beforehand:
 * 
 * <li>the TA function name;
 * <li>its input argument types;
 * <li>its output argument types;
 * <li>its optional input arguments types and domain values;
 * 
 * It means this class is mostly intended for test purposes and provided as example of how to obtain RTTI
 * (run time type information) from CoreMetaData.
 *  
 * @see com.tictactec.ta.lib.meta.CoreMetaData
 * 
 * @author Richard Gomes
 */
public class SimpleHelper {
    
    private String   func = null;
    private String[] args = null;
    private CoreMetaData calc = null;
    
    /**
     * Constructs a SimpleHelper class providing the TA function name and a list of optional parameters.
     * 
     * @see SimpleHelper#calculate(int, int, Object[], Object[], MInteger, MInteger)
     * @param func is the TA function name
     * @param args is a list of optional input arguments
     */
    public SimpleHelper(final String func, final List<String> args) {
        if (func==null || func.length()==0) throw new NullPointerException(); //TODO: message
        this.func = func;
        if (args!=null && args.size()>0) {
            this.args = (String[]) args.toArray(new String[0]);
            for (int i=0; i<this.args.length; i++) { this.args[i] = this.args[i].toUpperCase(); }
        }
    }

    /**
     * This method returns the underlying CoreMetaData class.
     * 
     * @return the underlying CoreMetaData class
     * @throws NoSuchMethodException
     * @throws IllegalArgumentException
     */
    public CoreMetaData getMetaData() throws NoSuchMethodException, IllegalArgumentException {
        if (this.calc!=null) return this.calc;
        this.calc = CoreMetaData.getInstance(func);
        if (args==null) return this.calc;
        FuncInfo finfo = calc.getFuncInfo();
        if (args.length>finfo.nbOptInput()) throw new IllegalArgumentException(); //TODO: message
        for (int i=0; i<args.length; i++) {
            OptInputParameterInfo ppinfo = calc.getOptInputParameterInfo(i);
            if (ppinfo.dataSet().isAssignableFrom(IntegerList.class) || ppinfo.dataSet().isAssignableFrom(IntegerRange.class)) {
                calc.setOptInputParamInteger(i, args[i]);
            } else if (ppinfo.dataSet().isAssignableFrom(RealList.class) || ppinfo.dataSet().isAssignableFrom(RealRange.class)) {
                calc.setOptInputParamReal(i, args[i]);
            } else {
                throw new ClassCastException(); //TODO: message
            }
        }
        return this.calc;
    }

    /**
     * Returns the lookback.
     * 
     * <p> Lookback is the number of input data points to be consumed in order to calculate the first output data point. This value
     * is affected by the optional input arguments passed to this TA function.
     *  
     * @return the lookback number of input points to be consumed before the first output data point is produced.
     * 
     * @throws NoSuchMethodException
     * @throws IllegalAccessException
     * @throws InvocationTargetException
     */
    public int getLookback() throws NoSuchMethodException, IllegalAccessException, InvocationTargetException {
        return getMetaData().getLookback();
    }
    
    /**
     * Executes the calculations defined by this TA function.
     * 
     * <p>You need to provide input arguments where this TA function will obtain data from and output arguments where this
     * TA function will write output data to. Optionally you can change default parameters used by this TA function in order
     * to execute the calculations. The typical use case would be:
     * <pre>
     *       func = "MAMA";
     *       params.clear();
     *       params.add("0.2");
     *       params.add("0.02");
     *       calc = new SimpleHelper(func, params);
     *       calc.calculate(0, 59, new Object[] { close }, new Object[] { output1, output2 }, lOutIdx, lOutSize);
     *       System.out.println("lookback="+calc.getLookback());
     *       System.out.println("outBegIdx    = "+lOutIdx.value+ "    outNbElement = "+lOutSize.value);
     *       for (int i=0; i<lOutSize.value; i++) {
     *           System.out.println("output1["+i+"]="+output1[i]+"     "+"output2["+i+"]="+output2[i]);
     *       }
     * </pre>
     * 
     * @param startIndex is the initial position of input data to be considered for TA function calculations
     * @param endIndex is the final position of input data to be considered for TA function calculations
     * @param inputs is an array of input arguments
     * @param outputs is an array of output arguments
     * @param outBegIdx is returned by this method and represents the initial position of output data returned by this TA function
     * @param outNbElement is returned by this method and represents the quantity of output data returned by this TA function
     * @throws IllegalArgumentException
     * @throws NoSuchMethodException
     * @throws IllegalAccessException
     * @throws InvocationTargetException
     */
    public void calculate(final int startIndex, final int endIndex,
            final Object[] inputs, Object[] outputs,
            MInteger outBegIdx, MInteger outNbElement)
                throws IllegalArgumentException, NoSuchMethodException, IllegalAccessException, InvocationTargetException {

        //for (int i=startIndex; i<startIndex+endIndex; i++) {
        //    System.err.println("input["+i+"]="+((double[])(inputs[0]))[i]);
        //}
        
        // parse function name and optional arguments
        FuncInfo finfo = getMetaData().getFuncInfo();
        // set input parameters
        if (inputs==null || inputs.length!=finfo.nbInput()) throw new IllegalArgumentException(); //TODO: message
        for (int i=0; i<inputs.length; i++) {
            InputParameterInfo ipinfo = calc.getInputParameterInfo(i);
            if (ipinfo.type()==InputParameterType.TA_Input_Price) {
                calc.setInputParamPrice(i, inputs[i]);
            } else if (ipinfo.type()==InputParameterType.TA_Input_Real) {
                calc.setInputParamReal(i, inputs[i]);
            } else if (ipinfo.type()==InputParameterType.TA_Input_Integer) {
                calc.setInputParamInteger(i, inputs[i]);
            } else {
                throw new IllegalArgumentException(); //TODO: message
            }
        }
        // set output parameters
        if (outputs==null || outputs.length!=finfo.nbOutput()) throw new IllegalArgumentException(); //TODO: message
        for (int i=0; i<outputs.length; i++) {
            OutputParameterInfo opinfo = calc.getOutputParameterInfo(i);
            if (opinfo.type()==OutputParameterType.TA_Output_Real) {
                calc.setOutputParamReal(i, outputs[i]);
            } else if (opinfo.type()==OutputParameterType.TA_Output_Integer) {
                calc.setOutputParamInteger(i, outputs[i]);
            } else {
                throw new IllegalArgumentException(); //TODO: message
            }
        }
        // call function
        calc.callFunc(startIndex, endIndex, outBegIdx, outNbElement); 
    }
    
}
