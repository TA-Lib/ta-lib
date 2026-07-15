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

package com.tictactec.ta.lib.test;

import java.util.ArrayList;
import java.util.List;
import java.util.Set;

import com.tictactec.ta.lib.MInteger;
import com.tictactec.ta.lib.meta.CoreMetaData;
import com.tictactec.ta.lib.meta.PriceHolder;
import com.tictactec.ta.lib.meta.TaFuncService;
import com.tictactec.ta.lib.meta.TaGrpService;
import com.tictactec.ta.lib.meta.annotation.InputParameterInfo;
import com.tictactec.ta.lib.meta.annotation.IntegerList;
import com.tictactec.ta.lib.meta.annotation.IntegerRange;
import com.tictactec.ta.lib.meta.annotation.OptInputParameterInfo;
import com.tictactec.ta.lib.meta.annotation.OutputParameterInfo;
import com.tictactec.ta.lib.meta.annotation.RealList;
import com.tictactec.ta.lib.meta.annotation.RealRange;
import com.tictactec.ta.lib.meta.helpers.SimpleHelper;

public class TestCoreMetaData {

    public static void main(String[] args) {
        
        final class DumpGrp implements TaGrpService {
            public void execute(String group, Set<CoreMetaData> set) {
                System.out.println("GROUP "+group);
                for (CoreMetaData mi : set) {
                    System.out.println("        "+mi.getFuncInfo().name());
                }
                
            }
        }
        
        final class DumpFunc implements TaFuncService {
            public void execute(CoreMetaData mi) {
                System.out.println(mi.getFuncInfo().name());
                for (int i = 0; i < mi.getFuncInfo().nbInput(); i++) {
                    InputParameterInfo pinfo = mi.getInputParameterInfo(i);
                    System.out.println("    " + pinfo.paramName());
                    System.out.println("        " + pinfo.type());
                }
                for (int i = 0; i < mi.getFuncInfo().nbOptInput(); i++) {
                    OptInputParameterInfo pinfo = mi.getOptInputParameterInfo(i);
                    System.out.println("    " + pinfo.paramName());
                    System.out.println("        " + pinfo.type());
                    switch (pinfo.type()) {
                    case TA_OptInput_RealRange:
                        RealRange rrange = mi.getOptInputRealRange(i);
                        System.out.println("            min="+rrange.min());
                        System.out.println("            max="+rrange.max());
                        System.out.println("            precision="+rrange.precision());
                        System.out.println("            default="+rrange.defaultValue());
                        break;
                    case TA_OptInput_RealList:
                        RealList rlist = mi.getOptInputRealList(i);
                        System.out.print("            value=");
                        for (double value : rlist.value()) {
                            System.out.print(value); System.out.print(" ");
                        }
                        System.out.println();
                        System.out.print("            string="+rlist.string());
                        for (String string : rlist.string()) {
                            System.out.print(string); System.out.print(" ");
                        }
                        System.out.println();
                        break;
                    case TA_OptInput_IntegerRange:
                        IntegerRange irange = mi.getOptInputIntegerRange(i);
                        System.out.println("            min="+irange.min());
                        System.out.println("            max="+irange.max());
                        System.out.println("            default="+irange.defaultValue());
                        break;
                    case TA_OptInput_IntegerList:
                        IntegerList ilist = mi.getOptInputIntegerList(i);
                        System.out.print("            value=");
                        for (int value : ilist.value()) {
                            System.out.print(value); System.out.print(" ");
                        }
                        System.out.println();
                        System.out.print("            string=");
                        for (String string : ilist.string()) {
                            System.out.print(string); System.out.print(" ");
                        }
                        System.out.println();
                        break;
                    }
                }
                for (int i = 0; i < mi.getFuncInfo().nbOutput(); i++) {
                    OutputParameterInfo pinfo = mi.getOutputParameterInfo(i);
                    System.out.println("    " + pinfo.paramName());
                    System.out.println("        " + pinfo.type());
                }
            }
        }

        TaGrpService grpServ = new DumpGrp();
        try {
            CoreMetaData.forEachGrp(grpServ);
        } catch (Exception e) {
            e.printStackTrace();
        }

        System.out.println("=  =  =  =  =  =  =  =  =  =  =  =  =");
        
        TaFuncService funcServ = new DumpFunc();
        try {
            CoreMetaData.forEachFunc(funcServ);
        } catch (Exception e) {
            e.printStackTrace();
        }
        System.out.println("=====================================");

    
    
        
        
        
        SimpleHelper calc;

        // input data
        double open[] = {
                1.4054, 1.4060, 1.4062, 1.4059, 1.4057, 1.4057, 1.4051, 1.4054, 1.4056, 1.4056,
                1.4054, 1.4060, 1.4062, 1.4059, 1.4057, 1.4057, 1.4051, 1.4054, 1.4056, 1.4056,
                1.4054, 1.4060, 1.4062, 1.4059, 1.4057, 1.4057, 1.4051, 1.4054, 1.4056, 1.4056,
                1.4054, 1.4060, 1.4062, 1.4059, 1.4057, 1.4057, 1.4051, 1.4054, 1.4056, 1.4056,
                1.4054, 1.4060, 1.4062, 1.4059, 1.4057, 1.4057, 1.4051, 1.4054, 1.4056, 1.4056,
                1.4054, 1.4060, 1.4062, 1.4059, 1.4057, 1.4057, 1.4051, 1.4054, 1.4056, 1.4056
        };

        double high[] = {
                1.4654, 1.4660, 1.4662, 1.4659, 1.4657, 1.4657, 1.4651, 1.4654, 1.4656, 1.4656,
                1.4654, 1.4660, 1.4662, 1.4659, 1.4657, 1.4657, 1.4651, 1.4654, 1.4656, 1.4656,
                1.4654, 1.4660, 1.4662, 1.4659, 1.4657, 1.4657, 1.4651, 1.4654, 1.4656, 1.4656,
                1.4654, 1.4660, 1.4662, 1.4659, 1.4657, 1.4657, 1.4651, 1.4654, 1.4656, 1.4656,
                1.4654, 1.4660, 1.4662, 1.4659, 1.4657, 1.4657, 1.4651, 1.4654, 1.4656, 1.4656,
                1.4654, 1.4660, 1.4662, 1.4659, 1.4657, 1.4657, 1.4651, 1.4654, 1.4656, 1.4656
        };


        double low[] = {
                1.3954, 1.3960, 1.3962, 1.3959, 1.3957, 1.3957, 1.3951, 1.3954, 1.3956, 1.3956,
                1.3954, 1.3960, 1.3962, 1.3959, 1.3957, 1.3957, 1.3951, 1.3954, 1.3956, 1.3956,
                1.3954, 1.3960, 1.3962, 1.3959, 1.3957, 1.3957, 1.3951, 1.3954, 1.3956, 1.3956,
                1.3954, 1.3960, 1.3962, 1.3959, 1.3957, 1.3957, 1.3951, 1.3954, 1.3956, 1.3956,
                1.3954, 1.3960, 1.3962, 1.3959, 1.3957, 1.3957, 1.3951, 1.3954, 1.3956, 1.3956,
                1.3954, 1.3960, 1.3962, 1.3959, 1.3957, 1.3957, 1.3951, 1.3954, 1.3956, 1.3956
        };

        
        double close[] = {
                1.4554, 1.4560, 1.4562, 1.4559, 1.4557, 1.4557, 1.4551, 1.4554, 1.4556, 1.4556,
                1.4554, 1.4560, 1.4562, 1.4559, 1.4557, 1.4557, 1.4551, 1.4554, 1.4556, 1.4556,
                1.4554, 1.4560, 1.4562, 1.4559, 1.4557, 1.4557, 1.4551, 1.4554, 1.4556, 1.4556,
                1.4554, 1.4560, 1.4562, 1.4559, 1.4557, 1.4557, 1.4551, 1.4554, 1.4556, 1.4556,
                1.4554, 1.4560, 1.4562, 1.4559, 1.4557, 1.4557, 1.4551, 1.4554, 1.4556, 1.4556,
                1.4554, 1.4560, 1.4562, 1.4559, 1.4557, 1.4557, 1.4551, 1.4554, 1.4556, 1.4556
        };
        // output buffers and return values
        double output1[] = new double[60];
        double output2[] = new double[60];
        double output3[] = new double[60];
        MInteger lOutIdx  = new MInteger();
        MInteger lOutSize = new MInteger();

        // function name and parameter holder
        String func;
        List<String> params = new ArrayList<String>();
        
        try {
            /*
                MAMA
                    [TA_Input_Real]    [0] inReal
                    [TA_OptInput_RealRange]   [0] optInFastLimit : 0.01..0.99 [0.01]
                    [TA_OptInput_RealRange]   [0] optInSlowLimit : 0.01..0.99 [0.01]
                    [TA_Output_Real]    [1] outMAMA
                    [TA_Output_Real]    [4] outFAMA
            */
            func = "MAMA";
            params.clear();
            params.add("0.2");
            params.add("0.02");
            calc = new SimpleHelper(func, params);
    
            System.out.println("===============================================");
            System.out.println(func);
            calc.calculate(0, 59, new Object[] { close }, new Object[] { output1, output2 }, lOutIdx, lOutSize);
            
            System.out.println("lookback="+calc.getLookback());
            System.out.println("outBegIdx    = "+lOutIdx.value+ "    outNbElement = "+lOutSize.value);
            for (int i=0; i<lOutSize.value; i++) {
                System.out.printf("output1[%2d]=%8.4f    output2[%2d]=%8.4f\n", i, output1[i], i, output2[i]);
                //System.out.println("output1["+i+"]="+output1[i]+"     "+"output2["+i+"]="+output2[i]);
            }

            /*
              BBANDS
                [TA_Input_Real]    [0] inReal
                [TA_OptInput_IntegerRange] [0] optInTimePeriod : 2..100000
                [TA_OptInput_RealRange]   [0] optInNbDevUp : -3e+37..3e+37 [0.01]
                [TA_OptInput_RealRange]   [0] optInNbDevDn : -3e+37..3e+37 [0.01]
                [TA_OptInput_IntegerList] [0] optInMAType : SMA=0, EMA=1, WMA=2, DEMA=3, TEMA=4, TRIMA=5, KAMA=6, MAMA=7, T3=8
                [TA_Output_Real]    [1] outRealUpperBand
                [TA_Output_Real]    [1] outRealMiddleBand
                [TA_Output_Real]    [1] outRealLowerBand
             */
            func = "bbands";
            params.clear();
            params.add("8");
            params.add("0.02");
            params.add("0.04");
            params.add("dEmA");
            
            calc = new SimpleHelper(func, params);
            
            System.out.println("===============================================");
            System.out.println(func);
            calc.calculate(0, 59, new Object[] { close }, new Object[] { output1, output2, output3 }, lOutIdx, lOutSize);
            
            System.out.println("lookback="+calc.getLookback());
            System.out.println("outBegIdx    = "+lOutIdx.value+ "    outNbElement = "+lOutSize.value);
            for (int i=0; i<lOutSize.value; i++) {
                System.out.printf("output1[%2d]=%8.4f    output2[%2d]=%8.4f        output3[%2d]=%8.4f\n", i, output1[i], i, output2[i], i, output3[i]);
                //System.out.println("output1["+i+"]="+output1[i]+"     "+"output2["+i+"]="+output2[i]+"     "+"output3["+i+"]="+output3[i]);
            }
            
            /*
                ADX
                    [TA_Input_Price]   [14] inPriceHLC
                    [TA_OptInput_IntegerRange] [0] optInTimePeriod : 2..100000
                    [TA_Output_Real]    [1] outReal             
            */
            func = "Adx";
            params.clear();
            params.add("8");
            
            calc = new SimpleHelper(func, params);
            
            System.out.println("===============================================");
            System.out.println(func);
            int flags = calc.getMetaData().getInputParameterInfo(0).flags();
            PriceHolder price = new PriceHolder(flags, open, high, low, close, null, null);
            calc.calculate(0, 59, new Object[] { price }, new Object[] { output1 }, lOutIdx, lOutSize);
            
            System.out.println("lookback="+calc.getLookback());
            System.out.println("outBegIdx    = "+lOutIdx.value+ "    outNbElement = "+lOutSize.value);
            for (int i=0; i<lOutSize.value; i++) {
                System.out.printf("output1[%2d]=%8.4f\n", i, output1[i]);
            }
            
            /*
              RSI
                [TA_Input_Real]    [0] inReal
                [TA_OptInput_IntegerRange] [0] optInTimePeriod : 2..100000
                [TA_Output_Real]    [1] outReal
             */
            func = "rsi";
            params.clear();
            params.add("8");
            
            calc = new SimpleHelper(func, params);
            
            System.out.println("===============================================");
            System.out.println(func);
            calc.calculate(0, 59, new Object[] { close }, new Object[] { output1 }, lOutIdx, lOutSize);
            
            System.out.println("lookback="+calc.getLookback());
            System.out.println("outBegIdx    = "+lOutIdx.value+ "    outNbElement = "+lOutSize.value);
            for (int i=0; i<lOutSize.value; i++) {
                System.out.printf("output1[%2d]=%8.4f\n", i, output1[i]);
                //System.out.println("output1["+i+"]="+output1[i]);
            }
            
            /*
            LN
              [TA_Input_Real]    [0] inReal
              [TA_Output_Real]   [1] outReal
           */
          func = "ln";
          params.clear();
          
          calc = new SimpleHelper(func, params);
          
          System.out.println("===============================================");
          System.out.println(func);
          calc.calculate(0, 59, new Object[] { close }, new Object[] { output1 }, lOutIdx, lOutSize);
          
          System.out.println("lookback="+calc.getLookback());
          System.out.println("outBegIdx    = "+lOutIdx.value+ "    outNbElement = "+lOutSize.value);
          for (int i=0; i<lOutSize.value; i++) {
              System.out.printf("output1[%2d]=%8.4f\n", i, output1[i]);
              //System.out.println("output1["+i+"]="+output1[i]);
          }
          
        } catch (Exception e) {
            e.printStackTrace();
        }
    
    }

}
