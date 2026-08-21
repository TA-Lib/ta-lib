/* TA-LIB Copyright (c) 1999-2026, Mario Fortier
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

/* GENERATED FILE — do not edit. Produced by ta_codegen
 * (generator/src/backends/java_metadata.rs) from ta_codegen/input/.
 * MF,CC
 */

package io.github.talib.metadata;

import java.util.Collections;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

/**
 * The catalogue of every TA-Lib indicator, for applications that pick a
 * function at run time — a charting UI enumerating indicators, a backtester
 * with user-selectable studies, a parameter sweep.
 *
 * <p>Generated from the same definitions as the indicators themselves, so it
 * cannot drift from them. Immutable and safe to use from any thread.
 *
 * <pre>{@code
 * for (FunctionInfo f : Functions.all()) {
 *     System.out.println(f.name() + " — " + f.hint());
 * }
 * }</pre>
 *
 * <p>Scope is the double-precision batch API — the same surface C's
 * {@code ta_abstract} and Rust's {@code abstract_api} describe. Streaming
 * handles and {@code float[]} overloads are not catalogued.
 */
public final class Functions {

    private Functions() { }

   private static final Map<String, FunctionInfo> BY_NAME = build();

   /** Every function, in canonical name order. */
   public static List<FunctionInfo> all() {
      return List.copyOf(BY_NAME.values());
   }

   /**
    * One function by canonical upper-case name, e.g. {@code "SMA"}.
    *
    * @return the metadata, or {@code null} if no such function exists
    */
   public static FunctionInfo byName(String name) {
      return BY_NAME.get(name);
   }

   /** The distinct group names, in first-appearance order. */
   public static List<String> groups() {
      return BY_NAME.values().stream().map(FunctionInfo::group).distinct().toList();
   }

   private static Map<String, FunctionInfo> build() {
      Map<String, FunctionInfo> m = new LinkedHashMap<>();
      put(m, f_AC());
      put(m, f_ACCBANDS());
      put(m, f_ACOS());
      put(m, f_AD());
      put(m, f_ADD());
      put(m, f_ADOSC());
      put(m, f_ADX());
      put(m, f_ADXR());
      put(m, f_AO());
      put(m, f_APO());
      put(m, f_AROON());
      put(m, f_AROONOSC());
      put(m, f_ASIN());
      put(m, f_ATAN());
      put(m, f_ATR());
      put(m, f_AVGDEV());
      put(m, f_AVGPRICE());
      put(m, f_BBANDS());
      put(m, f_BETA());
      put(m, f_BOP());
      put(m, f_CCI());
      put(m, f_CDL2CROWS());
      put(m, f_CDL3BLACKCROWS());
      put(m, f_CDL3INSIDE());
      put(m, f_CDL3LINESTRIKE());
      put(m, f_CDL3OUTSIDE());
      put(m, f_CDL3STARSINSOUTH());
      put(m, f_CDL3WHITESOLDIERS());
      put(m, f_CDLABANDONEDBABY());
      put(m, f_CDLADVANCEBLOCK());
      put(m, f_CDLBELTHOLD());
      put(m, f_CDLBREAKAWAY());
      put(m, f_CDLCLOSINGMARUBOZU());
      put(m, f_CDLCONCEALBABYSWALL());
      put(m, f_CDLCOUNTERATTACK());
      put(m, f_CDLDARKCLOUDCOVER());
      put(m, f_CDLDOJI());
      put(m, f_CDLDOJISTAR());
      put(m, f_CDLDRAGONFLYDOJI());
      put(m, f_CDLENGULFING());
      put(m, f_CDLEVENINGDOJISTAR());
      put(m, f_CDLEVENINGSTAR());
      put(m, f_CDLGAPSIDESIDEWHITE());
      put(m, f_CDLGRAVESTONEDOJI());
      put(m, f_CDLHAMMER());
      put(m, f_CDLHANGINGMAN());
      put(m, f_CDLHARAMI());
      put(m, f_CDLHARAMICROSS());
      put(m, f_CDLHIGHWAVE());
      put(m, f_CDLHIKKAKE());
      put(m, f_CDLHIKKAKEMOD());
      put(m, f_CDLHOMINGPIGEON());
      put(m, f_CDLIDENTICAL3CROWS());
      put(m, f_CDLINNECK());
      put(m, f_CDLINVERTEDHAMMER());
      put(m, f_CDLKICKING());
      put(m, f_CDLKICKINGBYLENGTH());
      put(m, f_CDLLADDERBOTTOM());
      put(m, f_CDLLONGLEGGEDDOJI());
      put(m, f_CDLLONGLINE());
      put(m, f_CDLMARUBOZU());
      put(m, f_CDLMATCHINGLOW());
      put(m, f_CDLMATHOLD());
      put(m, f_CDLMORNINGDOJISTAR());
      put(m, f_CDLMORNINGSTAR());
      put(m, f_CDLONNECK());
      put(m, f_CDLPIERCING());
      put(m, f_CDLRICKSHAWMAN());
      put(m, f_CDLRISEFALL3METHODS());
      put(m, f_CDLSEPARATINGLINES());
      put(m, f_CDLSHOOTINGSTAR());
      put(m, f_CDLSHORTLINE());
      put(m, f_CDLSPINNINGTOP());
      put(m, f_CDLSTALLEDPATTERN());
      put(m, f_CDLSTICKSANDWICH());
      put(m, f_CDLTAKURI());
      put(m, f_CDLTASUKIGAP());
      put(m, f_CDLTHRUSTING());
      put(m, f_CDLTRISTAR());
      put(m, f_CDLUNIQUE3RIVER());
      put(m, f_CDLUPSIDEGAP2CROWS());
      put(m, f_CDLXSIDEGAP3METHODS());
      put(m, f_CEIL());
      put(m, f_CMF());
      put(m, f_CMO());
      put(m, f_CMOU());
      put(m, f_CORREL());
      put(m, f_COS());
      put(m, f_COSH());
      put(m, f_DEMA());
      put(m, f_DIV());
      put(m, f_DX());
      put(m, f_EFI());
      put(m, f_EMA());
      put(m, f_EXP());
      put(m, f_FLOOR());
      put(m, f_HMA());
      put(m, f_HT_DCPERIOD());
      put(m, f_HT_DCPHASE());
      put(m, f_HT_PHASOR());
      put(m, f_HT_SINE());
      put(m, f_HT_TRENDLINE());
      put(m, f_HT_TRENDMODE());
      put(m, f_IMI());
      put(m, f_KAMA());
      put(m, f_LINEARREG());
      put(m, f_LINEARREG_ANGLE());
      put(m, f_LINEARREG_INTERCEPT());
      put(m, f_LINEARREG_SLOPE());
      put(m, f_LN());
      put(m, f_LOG10());
      put(m, f_MA());
      put(m, f_MACD());
      put(m, f_MACDEXT());
      put(m, f_MACDFIX());
      put(m, f_MAMA());
      put(m, f_MARKETFI());
      put(m, f_MAVP());
      put(m, f_MAX());
      put(m, f_MAXINDEX());
      put(m, f_MEDPRICE());
      put(m, f_MFI());
      put(m, f_MIDPOINT());
      put(m, f_MIDPRICE());
      put(m, f_MIN());
      put(m, f_MININDEX());
      put(m, f_MINMAX());
      put(m, f_MINMAXINDEX());
      put(m, f_MINUS_DI());
      put(m, f_MINUS_DM());
      put(m, f_MOM());
      put(m, f_MULT());
      put(m, f_NATR());
      put(m, f_NVI());
      put(m, f_OBV());
      put(m, f_PLUS_DI());
      put(m, f_PLUS_DM());
      put(m, f_PPO());
      put(m, f_PVI());
      put(m, f_PVO());
      put(m, f_QSTICK());
      put(m, f_ROC());
      put(m, f_ROCP());
      put(m, f_ROCR());
      put(m, f_ROCR100());
      put(m, f_RSI());
      put(m, f_SAR());
      put(m, f_SAREXT());
      put(m, f_SIN());
      put(m, f_SINH());
      put(m, f_SMA());
      put(m, f_SMI());
      put(m, f_SQRT());
      put(m, f_STDDEV());
      put(m, f_STOCH());
      put(m, f_STOCHF());
      put(m, f_STOCHRSI());
      put(m, f_SUB());
      put(m, f_SUM());
      put(m, f_T3());
      put(m, f_TAN());
      put(m, f_TANH());
      put(m, f_TEMA());
      put(m, f_TRANGE());
      put(m, f_TRIMA());
      put(m, f_TRIX());
      put(m, f_TSF());
      put(m, f_TYPPRICE());
      put(m, f_ULTOSC());
      put(m, f_VAR());
      put(m, f_VWMA());
      put(m, f_WAD());
      put(m, f_WCLPRICE());
      put(m, f_WILLR());
      put(m, f_WMA());
      return Collections.unmodifiableMap(m);
   }

   private static void put(Map<String, FunctionInfo> m, FunctionInfo f) {
      m.put(f.name(), f);
   }

   private static FunctionInfo f_AC() {
      return new FunctionInfo(
         "AC", "Momentum Indicators", "Accelerator/Decelerator Oscillator", 0x02000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceHL", 0x00000006)
         ),
         List.of(
            new OptInputInfo(
               OptInputType.INTEGER_RANGE, "optInFastPeriod", 0x00000000,
               "Fast Period", "Period of the fast MA", 5.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               2, 100000, 4, 200, 1, null),
            new OptInputInfo(
               OptInputType.INTEGER_RANGE, "optInSlowPeriod", 0x00000000,
               "Slow Period", "Period of the slow MA", 34.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               2, 100000, 4, 200, 1, null),
            new OptInputInfo(
               OptInputType.INTEGER_RANGE, "optInSignalPeriod", 0x00000000,
               "Signal Period", "Smoothing for the signal line (period length)", 5.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               2, 100000, 4, 200, 1, null)
         ),
         List.of(
            new OutputInfo(OutputType.REAL, "outReal", 0x00000010)
         ));
   }

   private static FunctionInfo f_ACCBANDS() {
      return new FunctionInfo(
         "ACCBANDS", "Overlap Studies", "Acceleration Bands", 0x03000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceHLC", 0x0000000E)
         ),
         List.of(
            new OptInputInfo(
               OptInputType.INTEGER_RANGE, "optInTimePeriod", 0x00000000,
               "Time Period", "Time period", 20.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               2, 100000, 4, 200, 1, null)
         ),
         List.of(
            new OutputInfo(OutputType.REAL, "outRealUpperBand", 0x00000800),
            new OutputInfo(OutputType.REAL, "outRealMiddleBand", 0x00000001),
            new OutputInfo(OutputType.REAL, "outRealLowerBand", 0x00001000)
         ));
   }

   private static FunctionInfo f_ACOS() {
      return new FunctionInfo(
         "ACOS", "Math Transform", "Vector Trigonometric ACos", 0x42000000,
         List.of(
            new InputInfo(InputType.REAL, "inReal", 0x00000000)
         ),
         List.of(),
         List.of(
            new OutputInfo(OutputType.REAL, "outReal", 0x00000001)
         ));
   }

   private static FunctionInfo f_AD() {
      return new FunctionInfo(
         "AD", "Volume Indicators", "Chaikin A/D Line", 0x22000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceHLCV", 0x0000001E)
         ),
         List.of(),
         List.of(
            new OutputInfo(OutputType.REAL, "outReal", 0x00000001)
         ));
   }

   private static FunctionInfo f_ADD() {
      return new FunctionInfo(
         "ADD", "Math Operators", "Vector Arithmetic Add", 0x02000000,
         List.of(
            new InputInfo(InputType.REAL, "inReal0", 0x00000000),
            new InputInfo(InputType.REAL, "inReal1", 0x00000000)
         ),
         List.of(),
         List.of(
            new OutputInfo(OutputType.REAL, "outReal", 0x00000001)
         ));
   }

   private static FunctionInfo f_ADOSC() {
      return new FunctionInfo(
         "ADOSC", "Volume Indicators", "Chaikin A/D Oscillator", 0x22000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceHLCV", 0x0000001E)
         ),
         List.of(
            new OptInputInfo(
               OptInputType.INTEGER_RANGE, "optInFastPeriod", 0x00000000,
               "Fast Period", "Period of the fast MA", 3.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               2, 100000, 4, 200, 1, null),
            new OptInputInfo(
               OptInputType.INTEGER_RANGE, "optInSlowPeriod", 0x00000000,
               "Slow Period", "Period of the slow MA", 10.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               2, 100000, 4, 200, 1, null)
         ),
         List.of(
            new OutputInfo(OutputType.REAL, "outReal", 0x00000001)
         ));
   }

   private static FunctionInfo f_ADX() {
      return new FunctionInfo(
         "ADX", "Momentum Indicators", "Average Directional Movement Index", 0x0A000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceHLC", 0x0000000E)
         ),
         List.of(
            new OptInputInfo(
               OptInputType.INTEGER_RANGE, "optInTimePeriod", 0x00000000,
               "Time Period", "Time period", 14.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               2, 100000, 4, 200, 1, null)
         ),
         List.of(
            new OutputInfo(OutputType.REAL, "outReal", 0x00000001)
         ));
   }

   private static FunctionInfo f_ADXR() {
      return new FunctionInfo(
         "ADXR", "Momentum Indicators", "Average Directional Movement Index Rating", 0x02000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceHLC", 0x0000000E)
         ),
         List.of(
            new OptInputInfo(
               OptInputType.INTEGER_RANGE, "optInTimePeriod", 0x00000000,
               "Time Period", "Time period", 14.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               2, 100000, 4, 200, 1, null)
         ),
         List.of(
            new OutputInfo(OutputType.REAL, "outReal", 0x00000001)
         ));
   }

   private static FunctionInfo f_AO() {
      return new FunctionInfo(
         "AO", "Momentum Indicators", "Awesome Oscillator", 0x02000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceHL", 0x00000006)
         ),
         List.of(
            new OptInputInfo(
               OptInputType.INTEGER_RANGE, "optInFastPeriod", 0x00000000,
               "Fast Period", "Period of the fast MA", 5.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               2, 100000, 4, 200, 1, null),
            new OptInputInfo(
               OptInputType.INTEGER_RANGE, "optInSlowPeriod", 0x00000000,
               "Slow Period", "Period of the slow MA", 34.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               2, 100000, 4, 200, 1, null)
         ),
         List.of(
            new OutputInfo(OutputType.REAL, "outReal", 0x00000010)
         ));
   }

   private static FunctionInfo f_APO() {
      return new FunctionInfo(
         "APO", "Momentum Indicators", "Absolute Price Oscillator", 0x02000000,
         List.of(
            new InputInfo(InputType.REAL, "inReal", 0x00000000)
         ),
         List.of(
            new OptInputInfo(
               OptInputType.INTEGER_RANGE, "optInFastPeriod", 0x00000000,
               "Fast Period", "Period of the fast MA", 12.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               2, 100000, 4, 200, 1, null),
            new OptInputInfo(
               OptInputType.INTEGER_RANGE, "optInSlowPeriod", 0x00000000,
               "Slow Period", "Period of the slow MA", 26.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               2, 100000, 4, 200, 1, null),
            new OptInputInfo(
               OptInputType.INTEGER_LIST, "optInMAType", 0x00000000,
               "MA Type", "Type of Moving Average", 1.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               0, 0, 0, 0, 0, "0=SMA;1=EMA;2=WMA;3=DEMA;4=TEMA;5=TRIMA;6=KAMA;7=MAMA;8=T3;9=HMA;10=DISABLED;11=DEFAULT")
         ),
         List.of(
            new OutputInfo(OutputType.REAL, "outReal", 0x00000001)
         ));
   }

   private static FunctionInfo f_AROON() {
      return new FunctionInfo(
         "AROON", "Momentum Indicators", "Aroon", 0x02000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceHL", 0x00000006)
         ),
         List.of(
            new OptInputInfo(
               OptInputType.INTEGER_RANGE, "optInTimePeriod", 0x00000000,
               "Time Period", "Time period", 14.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               2, 100000, 4, 200, 1, null)
         ),
         List.of(
            new OutputInfo(OutputType.REAL, "outAroonDown", 0x00000004),
            new OutputInfo(OutputType.REAL, "outAroonUp", 0x00000001)
         ));
   }

   private static FunctionInfo f_AROONOSC() {
      return new FunctionInfo(
         "AROONOSC", "Momentum Indicators", "Aroon Oscillator", 0x02000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceHL", 0x00000006)
         ),
         List.of(
            new OptInputInfo(
               OptInputType.INTEGER_RANGE, "optInTimePeriod", 0x00000000,
               "Time Period", "Time period", 14.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               2, 100000, 4, 200, 1, null)
         ),
         List.of(
            new OutputInfo(OutputType.REAL, "outReal", 0x00000001)
         ));
   }

   private static FunctionInfo f_ASIN() {
      return new FunctionInfo(
         "ASIN", "Math Transform", "Vector Trigonometric ASin", 0x42000000,
         List.of(
            new InputInfo(InputType.REAL, "inReal", 0x00000000)
         ),
         List.of(),
         List.of(
            new OutputInfo(OutputType.REAL, "outReal", 0x00000001)
         ));
   }

   private static FunctionInfo f_ATAN() {
      return new FunctionInfo(
         "ATAN", "Math Transform", "Vector Trigonometric ATan", 0x02000000,
         List.of(
            new InputInfo(InputType.REAL, "inReal", 0x00000000)
         ),
         List.of(),
         List.of(
            new OutputInfo(OutputType.REAL, "outReal", 0x00000001)
         ));
   }

   private static FunctionInfo f_ATR() {
      return new FunctionInfo(
         "ATR", "Volatility Indicators", "Average True Range", 0x0A000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceHLC", 0x0000000E)
         ),
         List.of(
            new OptInputInfo(
               OptInputType.INTEGER_RANGE, "optInTimePeriod", 0x00000000,
               "Time Period", "Time period", 14.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               1, 100000, 1, 200, 1, null)
         ),
         List.of(
            new OutputInfo(OutputType.REAL, "outReal", 0x00000001)
         ));
   }

   private static FunctionInfo f_AVGDEV() {
      return new FunctionInfo(
         "AVGDEV", "Price Transform", "Average Deviation", 0x03000000,
         List.of(
            new InputInfo(InputType.REAL, "inReal", 0x00000000)
         ),
         List.of(
            new OptInputInfo(
               OptInputType.INTEGER_RANGE, "optInTimePeriod", 0x00000000,
               "Time Period", "Time period", 14.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               2, 100000, 4, 200, 1, null)
         ),
         List.of(
            new OutputInfo(OutputType.REAL, "outReal", 0x00000001)
         ));
   }

   private static FunctionInfo f_AVGPRICE() {
      return new FunctionInfo(
         "AVGPRICE", "Price Transform", "Average Price", 0x03000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceOHLC", 0x0000000F)
         ),
         List.of(),
         List.of(
            new OutputInfo(OutputType.REAL, "outReal", 0x00000001)
         ));
   }

   private static FunctionInfo f_BBANDS() {
      return new FunctionInfo(
         "BBANDS", "Overlap Studies", "Bollinger Bands", 0x03000000,
         List.of(
            new InputInfo(InputType.REAL, "inReal", 0x00000000)
         ),
         List.of(
            new OptInputInfo(
               OptInputType.INTEGER_RANGE, "optInTimePeriod", 0x00000000,
               "Time Period", "Time period", 20.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               2, 100000, 4, 200, 1, null),
            new OptInputInfo(
               OptInputType.REAL_RANGE, "optInNbDevUp", 0x00000000,
               "Deviations up", "Deviation multiplier for upper band", 2.0,
               -3e37, 3e37, 2, -2.0, 2.0, 0.2,
               0, 0, 0, 0, 0, null),
            new OptInputInfo(
               OptInputType.REAL_RANGE, "optInNbDevDn", 0x00000000,
               "Deviations down", "Deviation multiplier for lower band", 2.0,
               -3e37, 3e37, 2, -2.0, 2.0, 0.2,
               0, 0, 0, 0, 0, null),
            new OptInputInfo(
               OptInputType.INTEGER_LIST, "optInMAType", 0x00000000,
               "MA Type", "Type of Moving Average", 0.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               0, 0, 0, 0, 0, "0=SMA;1=EMA;2=WMA;3=DEMA;4=TEMA;5=TRIMA;6=KAMA;7=MAMA;8=T3;9=HMA;10=DISABLED;11=DEFAULT")
         ),
         List.of(
            new OutputInfo(OutputType.REAL, "outRealUpperBand", 0x00000800),
            new OutputInfo(OutputType.REAL, "outRealMiddleBand", 0x00000001),
            new OutputInfo(OutputType.REAL, "outRealLowerBand", 0x00001000)
         ));
   }

   private static FunctionInfo f_BETA() {
      return new FunctionInfo(
         "BETA", "Statistic Functions", "Beta", 0x02000000,
         List.of(
            new InputInfo(InputType.REAL, "inReal0", 0x00000000),
            new InputInfo(InputType.REAL, "inReal1", 0x00000000)
         ),
         List.of(
            new OptInputInfo(
               OptInputType.INTEGER_RANGE, "optInTimePeriod", 0x00000000,
               "Time Period", "Time period", 5.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               1, 100000, 1, 200, 1, null)
         ),
         List.of(
            new OutputInfo(OutputType.REAL, "outReal", 0x00000001)
         ));
   }

   private static FunctionInfo f_BOP() {
      return new FunctionInfo(
         "BOP", "Momentum Indicators", "Balance Of Power", 0x02000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceOHLC", 0x0000000F)
         ),
         List.of(),
         List.of(
            new OutputInfo(OutputType.REAL, "outReal", 0x00000001)
         ));
   }

   private static FunctionInfo f_CCI() {
      return new FunctionInfo(
         "CCI", "Momentum Indicators", "Commodity Channel Index", 0x02000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceHLC", 0x0000000E)
         ),
         List.of(
            new OptInputInfo(
               OptInputType.INTEGER_RANGE, "optInTimePeriod", 0x00000000,
               "Time Period", "Time period", 14.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               2, 100000, 4, 200, 1, null)
         ),
         List.of(
            new OutputInfo(OutputType.REAL, "outReal", 0x00000001)
         ));
   }

   private static FunctionInfo f_CDL2CROWS() {
      return new FunctionInfo(
         "CDL2CROWS", "Pattern Recognition", "Two Crows", 0x12000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceOHLC", 0x0000000F)
         ),
         List.of(),
         List.of(
            new OutputInfo(OutputType.INTEGER, "outInteger", 0x00000001)
         ));
   }

   private static FunctionInfo f_CDL3BLACKCROWS() {
      return new FunctionInfo(
         "CDL3BLACKCROWS", "Pattern Recognition", "Three Black Crows", 0x12000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceOHLC", 0x0000000F)
         ),
         List.of(),
         List.of(
            new OutputInfo(OutputType.INTEGER, "outInteger", 0x00000001)
         ));
   }

   private static FunctionInfo f_CDL3INSIDE() {
      return new FunctionInfo(
         "CDL3INSIDE", "Pattern Recognition", "Three Inside Up/Down", 0x12000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceOHLC", 0x0000000F)
         ),
         List.of(),
         List.of(
            new OutputInfo(OutputType.INTEGER, "outInteger", 0x00000001)
         ));
   }

   private static FunctionInfo f_CDL3LINESTRIKE() {
      return new FunctionInfo(
         "CDL3LINESTRIKE", "Pattern Recognition", "Three-Line Strike", 0x12000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceOHLC", 0x0000000F)
         ),
         List.of(),
         List.of(
            new OutputInfo(OutputType.INTEGER, "outInteger", 0x00000001)
         ));
   }

   private static FunctionInfo f_CDL3OUTSIDE() {
      return new FunctionInfo(
         "CDL3OUTSIDE", "Pattern Recognition", "Three Outside Up/Down", 0x12000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceOHLC", 0x0000000F)
         ),
         List.of(),
         List.of(
            new OutputInfo(OutputType.INTEGER, "outInteger", 0x00000001)
         ));
   }

   private static FunctionInfo f_CDL3STARSINSOUTH() {
      return new FunctionInfo(
         "CDL3STARSINSOUTH", "Pattern Recognition", "Three Stars In The South", 0x12000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceOHLC", 0x0000000F)
         ),
         List.of(),
         List.of(
            new OutputInfo(OutputType.INTEGER, "outInteger", 0x00000001)
         ));
   }

   private static FunctionInfo f_CDL3WHITESOLDIERS() {
      return new FunctionInfo(
         "CDL3WHITESOLDIERS", "Pattern Recognition", "Three Advancing White Soldiers", 0x12000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceOHLC", 0x0000000F)
         ),
         List.of(),
         List.of(
            new OutputInfo(OutputType.INTEGER, "outInteger", 0x00000001)
         ));
   }

   private static FunctionInfo f_CDLABANDONEDBABY() {
      return new FunctionInfo(
         "CDLABANDONEDBABY", "Pattern Recognition", "Abandoned Baby", 0x12000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceOHLC", 0x0000000F)
         ),
         List.of(
            new OptInputInfo(
               OptInputType.REAL_RANGE, "optInPenetration", 0x00000000,
               "Penetration", "Percentage of penetration of a candle within another candle", 0.3,
               0.0, 3e37, 0, 0.0, 0.0, 0.0,
               0, 0, 0, 0, 0, null)
         ),
         List.of(
            new OutputInfo(OutputType.INTEGER, "outInteger", 0x00000001)
         ));
   }

   private static FunctionInfo f_CDLADVANCEBLOCK() {
      return new FunctionInfo(
         "CDLADVANCEBLOCK", "Pattern Recognition", "Advance Block", 0x12000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceOHLC", 0x0000000F)
         ),
         List.of(),
         List.of(
            new OutputInfo(OutputType.INTEGER, "outInteger", 0x00000001)
         ));
   }

   private static FunctionInfo f_CDLBELTHOLD() {
      return new FunctionInfo(
         "CDLBELTHOLD", "Pattern Recognition", "Belt-hold", 0x12000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceOHLC", 0x0000000F)
         ),
         List.of(),
         List.of(
            new OutputInfo(OutputType.INTEGER, "outInteger", 0x00000001)
         ));
   }

   private static FunctionInfo f_CDLBREAKAWAY() {
      return new FunctionInfo(
         "CDLBREAKAWAY", "Pattern Recognition", "Breakaway", 0x12000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceOHLC", 0x0000000F)
         ),
         List.of(),
         List.of(
            new OutputInfo(OutputType.INTEGER, "outInteger", 0x00000001)
         ));
   }

   private static FunctionInfo f_CDLCLOSINGMARUBOZU() {
      return new FunctionInfo(
         "CDLCLOSINGMARUBOZU", "Pattern Recognition", "Closing Marubozu", 0x12000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceOHLC", 0x0000000F)
         ),
         List.of(),
         List.of(
            new OutputInfo(OutputType.INTEGER, "outInteger", 0x00000001)
         ));
   }

   private static FunctionInfo f_CDLCONCEALBABYSWALL() {
      return new FunctionInfo(
         "CDLCONCEALBABYSWALL", "Pattern Recognition", "Concealing Baby Swallow", 0x12000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceOHLC", 0x0000000F)
         ),
         List.of(),
         List.of(
            new OutputInfo(OutputType.INTEGER, "outInteger", 0x00000001)
         ));
   }

   private static FunctionInfo f_CDLCOUNTERATTACK() {
      return new FunctionInfo(
         "CDLCOUNTERATTACK", "Pattern Recognition", "Counterattack", 0x12000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceOHLC", 0x0000000F)
         ),
         List.of(),
         List.of(
            new OutputInfo(OutputType.INTEGER, "outInteger", 0x00000001)
         ));
   }

   private static FunctionInfo f_CDLDARKCLOUDCOVER() {
      return new FunctionInfo(
         "CDLDARKCLOUDCOVER", "Pattern Recognition", "Dark Cloud Cover", 0x12000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceOHLC", 0x0000000F)
         ),
         List.of(
            new OptInputInfo(
               OptInputType.REAL_RANGE, "optInPenetration", 0x00000000,
               "Penetration", "Percentage of penetration of a candle within another candle", 0.5,
               0.0, 3e37, 0, 0.0, 0.0, 0.0,
               0, 0, 0, 0, 0, null)
         ),
         List.of(
            new OutputInfo(OutputType.INTEGER, "outInteger", 0x00000001)
         ));
   }

   private static FunctionInfo f_CDLDOJI() {
      return new FunctionInfo(
         "CDLDOJI", "Pattern Recognition", "Doji", 0x12000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceOHLC", 0x0000000F)
         ),
         List.of(),
         List.of(
            new OutputInfo(OutputType.INTEGER, "outInteger", 0x00000001)
         ));
   }

   private static FunctionInfo f_CDLDOJISTAR() {
      return new FunctionInfo(
         "CDLDOJISTAR", "Pattern Recognition", "Doji Star", 0x12000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceOHLC", 0x0000000F)
         ),
         List.of(),
         List.of(
            new OutputInfo(OutputType.INTEGER, "outInteger", 0x00000001)
         ));
   }

   private static FunctionInfo f_CDLDRAGONFLYDOJI() {
      return new FunctionInfo(
         "CDLDRAGONFLYDOJI", "Pattern Recognition", "Dragonfly Doji", 0x12000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceOHLC", 0x0000000F)
         ),
         List.of(),
         List.of(
            new OutputInfo(OutputType.INTEGER, "outInteger", 0x00000001)
         ));
   }

   private static FunctionInfo f_CDLENGULFING() {
      return new FunctionInfo(
         "CDLENGULFING", "Pattern Recognition", "Engulfing Pattern", 0x12000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceOHLC", 0x0000000F)
         ),
         List.of(),
         List.of(
            new OutputInfo(OutputType.INTEGER, "outInteger", 0x00000001)
         ));
   }

   private static FunctionInfo f_CDLEVENINGDOJISTAR() {
      return new FunctionInfo(
         "CDLEVENINGDOJISTAR", "Pattern Recognition", "Evening Doji Star", 0x12000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceOHLC", 0x0000000F)
         ),
         List.of(
            new OptInputInfo(
               OptInputType.REAL_RANGE, "optInPenetration", 0x00000000,
               "Penetration", "Percentage of penetration of a candle within another candle", 0.3,
               0.0, 3e37, 0, 0.0, 0.0, 0.0,
               0, 0, 0, 0, 0, null)
         ),
         List.of(
            new OutputInfo(OutputType.INTEGER, "outInteger", 0x00000001)
         ));
   }

   private static FunctionInfo f_CDLEVENINGSTAR() {
      return new FunctionInfo(
         "CDLEVENINGSTAR", "Pattern Recognition", "Evening Star", 0x12000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceOHLC", 0x0000000F)
         ),
         List.of(
            new OptInputInfo(
               OptInputType.REAL_RANGE, "optInPenetration", 0x00000000,
               "Penetration", "Percentage of penetration of a candle within another candle", 0.3,
               0.0, 3e37, 0, 0.0, 0.0, 0.0,
               0, 0, 0, 0, 0, null)
         ),
         List.of(
            new OutputInfo(OutputType.INTEGER, "outInteger", 0x00000001)
         ));
   }

   private static FunctionInfo f_CDLGAPSIDESIDEWHITE() {
      return new FunctionInfo(
         "CDLGAPSIDESIDEWHITE", "Pattern Recognition", "Up/Down-gap side-by-side white lines", 0x12000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceOHLC", 0x0000000F)
         ),
         List.of(),
         List.of(
            new OutputInfo(OutputType.INTEGER, "outInteger", 0x00000001)
         ));
   }

   private static FunctionInfo f_CDLGRAVESTONEDOJI() {
      return new FunctionInfo(
         "CDLGRAVESTONEDOJI", "Pattern Recognition", "Gravestone Doji", 0x12000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceOHLC", 0x0000000F)
         ),
         List.of(),
         List.of(
            new OutputInfo(OutputType.INTEGER, "outInteger", 0x00000001)
         ));
   }

   private static FunctionInfo f_CDLHAMMER() {
      return new FunctionInfo(
         "CDLHAMMER", "Pattern Recognition", "Hammer", 0x12000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceOHLC", 0x0000000F)
         ),
         List.of(),
         List.of(
            new OutputInfo(OutputType.INTEGER, "outInteger", 0x00000001)
         ));
   }

   private static FunctionInfo f_CDLHANGINGMAN() {
      return new FunctionInfo(
         "CDLHANGINGMAN", "Pattern Recognition", "Hanging Man", 0x12000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceOHLC", 0x0000000F)
         ),
         List.of(),
         List.of(
            new OutputInfo(OutputType.INTEGER, "outInteger", 0x00000001)
         ));
   }

   private static FunctionInfo f_CDLHARAMI() {
      return new FunctionInfo(
         "CDLHARAMI", "Pattern Recognition", "Harami Pattern", 0x12000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceOHLC", 0x0000000F)
         ),
         List.of(),
         List.of(
            new OutputInfo(OutputType.INTEGER, "outInteger", 0x00000001)
         ));
   }

   private static FunctionInfo f_CDLHARAMICROSS() {
      return new FunctionInfo(
         "CDLHARAMICROSS", "Pattern Recognition", "Harami Cross Pattern", 0x12000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceOHLC", 0x0000000F)
         ),
         List.of(),
         List.of(
            new OutputInfo(OutputType.INTEGER, "outInteger", 0x00000001)
         ));
   }

   private static FunctionInfo f_CDLHIGHWAVE() {
      return new FunctionInfo(
         "CDLHIGHWAVE", "Pattern Recognition", "High-Wave Candle", 0x12000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceOHLC", 0x0000000F)
         ),
         List.of(),
         List.of(
            new OutputInfo(OutputType.INTEGER, "outInteger", 0x00000001)
         ));
   }

   private static FunctionInfo f_CDLHIKKAKE() {
      return new FunctionInfo(
         "CDLHIKKAKE", "Pattern Recognition", "Hikkake Pattern", 0x12000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceOHLC", 0x0000000F)
         ),
         List.of(),
         List.of(
            new OutputInfo(OutputType.INTEGER, "outInteger", 0x00000001)
         ));
   }

   private static FunctionInfo f_CDLHIKKAKEMOD() {
      return new FunctionInfo(
         "CDLHIKKAKEMOD", "Pattern Recognition", "Modified Hikkake Pattern", 0x12000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceOHLC", 0x0000000F)
         ),
         List.of(),
         List.of(
            new OutputInfo(OutputType.INTEGER, "outInteger", 0x00000001)
         ));
   }

   private static FunctionInfo f_CDLHOMINGPIGEON() {
      return new FunctionInfo(
         "CDLHOMINGPIGEON", "Pattern Recognition", "Homing Pigeon", 0x12000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceOHLC", 0x0000000F)
         ),
         List.of(),
         List.of(
            new OutputInfo(OutputType.INTEGER, "outInteger", 0x00000001)
         ));
   }

   private static FunctionInfo f_CDLIDENTICAL3CROWS() {
      return new FunctionInfo(
         "CDLIDENTICAL3CROWS", "Pattern Recognition", "Identical Three Crows", 0x12000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceOHLC", 0x0000000F)
         ),
         List.of(),
         List.of(
            new OutputInfo(OutputType.INTEGER, "outInteger", 0x00000001)
         ));
   }

   private static FunctionInfo f_CDLINNECK() {
      return new FunctionInfo(
         "CDLINNECK", "Pattern Recognition", "In-Neck Pattern", 0x12000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceOHLC", 0x0000000F)
         ),
         List.of(),
         List.of(
            new OutputInfo(OutputType.INTEGER, "outInteger", 0x00000001)
         ));
   }

   private static FunctionInfo f_CDLINVERTEDHAMMER() {
      return new FunctionInfo(
         "CDLINVERTEDHAMMER", "Pattern Recognition", "Inverted Hammer", 0x12000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceOHLC", 0x0000000F)
         ),
         List.of(),
         List.of(
            new OutputInfo(OutputType.INTEGER, "outInteger", 0x00000001)
         ));
   }

   private static FunctionInfo f_CDLKICKING() {
      return new FunctionInfo(
         "CDLKICKING", "Pattern Recognition", "Kicking", 0x12000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceOHLC", 0x0000000F)
         ),
         List.of(),
         List.of(
            new OutputInfo(OutputType.INTEGER, "outInteger", 0x00000001)
         ));
   }

   private static FunctionInfo f_CDLKICKINGBYLENGTH() {
      return new FunctionInfo(
         "CDLKICKINGBYLENGTH", "Pattern Recognition", "Kicking - bull/bear determined by the longer marubozu", 0x12000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceOHLC", 0x0000000F)
         ),
         List.of(),
         List.of(
            new OutputInfo(OutputType.INTEGER, "outInteger", 0x00000001)
         ));
   }

   private static FunctionInfo f_CDLLADDERBOTTOM() {
      return new FunctionInfo(
         "CDLLADDERBOTTOM", "Pattern Recognition", "Ladder Bottom", 0x12000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceOHLC", 0x0000000F)
         ),
         List.of(),
         List.of(
            new OutputInfo(OutputType.INTEGER, "outInteger", 0x00000001)
         ));
   }

   private static FunctionInfo f_CDLLONGLEGGEDDOJI() {
      return new FunctionInfo(
         "CDLLONGLEGGEDDOJI", "Pattern Recognition", "Long Legged Doji", 0x12000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceOHLC", 0x0000000F)
         ),
         List.of(),
         List.of(
            new OutputInfo(OutputType.INTEGER, "outInteger", 0x00000001)
         ));
   }

   private static FunctionInfo f_CDLLONGLINE() {
      return new FunctionInfo(
         "CDLLONGLINE", "Pattern Recognition", "Long Line Candle", 0x12000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceOHLC", 0x0000000F)
         ),
         List.of(),
         List.of(
            new OutputInfo(OutputType.INTEGER, "outInteger", 0x00000001)
         ));
   }

   private static FunctionInfo f_CDLMARUBOZU() {
      return new FunctionInfo(
         "CDLMARUBOZU", "Pattern Recognition", "Marubozu", 0x12000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceOHLC", 0x0000000F)
         ),
         List.of(),
         List.of(
            new OutputInfo(OutputType.INTEGER, "outInteger", 0x00000001)
         ));
   }

   private static FunctionInfo f_CDLMATCHINGLOW() {
      return new FunctionInfo(
         "CDLMATCHINGLOW", "Pattern Recognition", "Matching Low", 0x12000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceOHLC", 0x0000000F)
         ),
         List.of(),
         List.of(
            new OutputInfo(OutputType.INTEGER, "outInteger", 0x00000001)
         ));
   }

   private static FunctionInfo f_CDLMATHOLD() {
      return new FunctionInfo(
         "CDLMATHOLD", "Pattern Recognition", "Mat Hold", 0x12000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceOHLC", 0x0000000F)
         ),
         List.of(
            new OptInputInfo(
               OptInputType.REAL_RANGE, "optInPenetration", 0x00000000,
               "Penetration", "Percentage of penetration of a candle within another candle", 0.5,
               0.0, 3e37, 0, 0.0, 0.0, 0.0,
               0, 0, 0, 0, 0, null)
         ),
         List.of(
            new OutputInfo(OutputType.INTEGER, "outInteger", 0x00000001)
         ));
   }

   private static FunctionInfo f_CDLMORNINGDOJISTAR() {
      return new FunctionInfo(
         "CDLMORNINGDOJISTAR", "Pattern Recognition", "Morning Doji Star", 0x12000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceOHLC", 0x0000000F)
         ),
         List.of(
            new OptInputInfo(
               OptInputType.REAL_RANGE, "optInPenetration", 0x00000000,
               "Penetration", "Percentage of penetration of a candle within another candle", 0.3,
               0.0, 3e37, 0, 0.0, 0.0, 0.0,
               0, 0, 0, 0, 0, null)
         ),
         List.of(
            new OutputInfo(OutputType.INTEGER, "outInteger", 0x00000001)
         ));
   }

   private static FunctionInfo f_CDLMORNINGSTAR() {
      return new FunctionInfo(
         "CDLMORNINGSTAR", "Pattern Recognition", "Morning Star", 0x12000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceOHLC", 0x0000000F)
         ),
         List.of(
            new OptInputInfo(
               OptInputType.REAL_RANGE, "optInPenetration", 0x00000000,
               "Penetration", "Percentage of penetration of a candle within another candle", 0.3,
               0.0, 3e37, 0, 0.0, 0.0, 0.0,
               0, 0, 0, 0, 0, null)
         ),
         List.of(
            new OutputInfo(OutputType.INTEGER, "outInteger", 0x00000001)
         ));
   }

   private static FunctionInfo f_CDLONNECK() {
      return new FunctionInfo(
         "CDLONNECK", "Pattern Recognition", "On-Neck Pattern", 0x12000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceOHLC", 0x0000000F)
         ),
         List.of(),
         List.of(
            new OutputInfo(OutputType.INTEGER, "outInteger", 0x00000001)
         ));
   }

   private static FunctionInfo f_CDLPIERCING() {
      return new FunctionInfo(
         "CDLPIERCING", "Pattern Recognition", "Piercing Pattern", 0x12000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceOHLC", 0x0000000F)
         ),
         List.of(),
         List.of(
            new OutputInfo(OutputType.INTEGER, "outInteger", 0x00000001)
         ));
   }

   private static FunctionInfo f_CDLRICKSHAWMAN() {
      return new FunctionInfo(
         "CDLRICKSHAWMAN", "Pattern Recognition", "Rickshaw Man", 0x12000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceOHLC", 0x0000000F)
         ),
         List.of(),
         List.of(
            new OutputInfo(OutputType.INTEGER, "outInteger", 0x00000001)
         ));
   }

   private static FunctionInfo f_CDLRISEFALL3METHODS() {
      return new FunctionInfo(
         "CDLRISEFALL3METHODS", "Pattern Recognition", "Rising/Falling Three Methods", 0x12000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceOHLC", 0x0000000F)
         ),
         List.of(),
         List.of(
            new OutputInfo(OutputType.INTEGER, "outInteger", 0x00000001)
         ));
   }

   private static FunctionInfo f_CDLSEPARATINGLINES() {
      return new FunctionInfo(
         "CDLSEPARATINGLINES", "Pattern Recognition", "Separating Lines", 0x12000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceOHLC", 0x0000000F)
         ),
         List.of(),
         List.of(
            new OutputInfo(OutputType.INTEGER, "outInteger", 0x00000001)
         ));
   }

   private static FunctionInfo f_CDLSHOOTINGSTAR() {
      return new FunctionInfo(
         "CDLSHOOTINGSTAR", "Pattern Recognition", "Shooting Star", 0x12000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceOHLC", 0x0000000F)
         ),
         List.of(),
         List.of(
            new OutputInfo(OutputType.INTEGER, "outInteger", 0x00000001)
         ));
   }

   private static FunctionInfo f_CDLSHORTLINE() {
      return new FunctionInfo(
         "CDLSHORTLINE", "Pattern Recognition", "Short Line Candle", 0x12000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceOHLC", 0x0000000F)
         ),
         List.of(),
         List.of(
            new OutputInfo(OutputType.INTEGER, "outInteger", 0x00000001)
         ));
   }

   private static FunctionInfo f_CDLSPINNINGTOP() {
      return new FunctionInfo(
         "CDLSPINNINGTOP", "Pattern Recognition", "Spinning Top", 0x12000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceOHLC", 0x0000000F)
         ),
         List.of(),
         List.of(
            new OutputInfo(OutputType.INTEGER, "outInteger", 0x00000001)
         ));
   }

   private static FunctionInfo f_CDLSTALLEDPATTERN() {
      return new FunctionInfo(
         "CDLSTALLEDPATTERN", "Pattern Recognition", "Stalled Pattern", 0x12000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceOHLC", 0x0000000F)
         ),
         List.of(),
         List.of(
            new OutputInfo(OutputType.INTEGER, "outInteger", 0x00000001)
         ));
   }

   private static FunctionInfo f_CDLSTICKSANDWICH() {
      return new FunctionInfo(
         "CDLSTICKSANDWICH", "Pattern Recognition", "Stick Sandwich", 0x12000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceOHLC", 0x0000000F)
         ),
         List.of(),
         List.of(
            new OutputInfo(OutputType.INTEGER, "outInteger", 0x00000001)
         ));
   }

   private static FunctionInfo f_CDLTAKURI() {
      return new FunctionInfo(
         "CDLTAKURI", "Pattern Recognition", "Takuri (Dragonfly Doji with very long lower shadow)", 0x12000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceOHLC", 0x0000000F)
         ),
         List.of(),
         List.of(
            new OutputInfo(OutputType.INTEGER, "outInteger", 0x00000001)
         ));
   }

   private static FunctionInfo f_CDLTASUKIGAP() {
      return new FunctionInfo(
         "CDLTASUKIGAP", "Pattern Recognition", "Tasuki Gap", 0x12000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceOHLC", 0x0000000F)
         ),
         List.of(),
         List.of(
            new OutputInfo(OutputType.INTEGER, "outInteger", 0x00000001)
         ));
   }

   private static FunctionInfo f_CDLTHRUSTING() {
      return new FunctionInfo(
         "CDLTHRUSTING", "Pattern Recognition", "Thrusting Pattern", 0x12000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceOHLC", 0x0000000F)
         ),
         List.of(),
         List.of(
            new OutputInfo(OutputType.INTEGER, "outInteger", 0x00000001)
         ));
   }

   private static FunctionInfo f_CDLTRISTAR() {
      return new FunctionInfo(
         "CDLTRISTAR", "Pattern Recognition", "Tristar Pattern", 0x12000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceOHLC", 0x0000000F)
         ),
         List.of(),
         List.of(
            new OutputInfo(OutputType.INTEGER, "outInteger", 0x00000001)
         ));
   }

   private static FunctionInfo f_CDLUNIQUE3RIVER() {
      return new FunctionInfo(
         "CDLUNIQUE3RIVER", "Pattern Recognition", "Unique 3 River", 0x12000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceOHLC", 0x0000000F)
         ),
         List.of(),
         List.of(
            new OutputInfo(OutputType.INTEGER, "outInteger", 0x00000001)
         ));
   }

   private static FunctionInfo f_CDLUPSIDEGAP2CROWS() {
      return new FunctionInfo(
         "CDLUPSIDEGAP2CROWS", "Pattern Recognition", "Upside Gap Two Crows", 0x12000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceOHLC", 0x0000000F)
         ),
         List.of(),
         List.of(
            new OutputInfo(OutputType.INTEGER, "outInteger", 0x00000001)
         ));
   }

   private static FunctionInfo f_CDLXSIDEGAP3METHODS() {
      return new FunctionInfo(
         "CDLXSIDEGAP3METHODS", "Pattern Recognition", "Upside/Downside Gap Three Methods", 0x12000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceOHLC", 0x0000000F)
         ),
         List.of(),
         List.of(
            new OutputInfo(OutputType.INTEGER, "outInteger", 0x00000001)
         ));
   }

   private static FunctionInfo f_CEIL() {
      return new FunctionInfo(
         "CEIL", "Math Transform", "Vector Ceil", 0x02000000,
         List.of(
            new InputInfo(InputType.REAL, "inReal", 0x00000000)
         ),
         List.of(),
         List.of(
            new OutputInfo(OutputType.REAL, "outReal", 0x00000001)
         ));
   }

   private static FunctionInfo f_CMF() {
      return new FunctionInfo(
         "CMF", "Volume Indicators", "Chaikin Money Flow", 0x02000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceHLCV", 0x0000001E)
         ),
         List.of(
            new OptInputInfo(
               OptInputType.INTEGER_RANGE, "optInTimePeriod", 0x00000000,
               "Time Period", "Time period", 20.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               2, 100000, 4, 200, 1, null)
         ),
         List.of(
            new OutputInfo(OutputType.REAL, "outReal", 0x00000001)
         ));
   }

   private static FunctionInfo f_CMO() {
      return new FunctionInfo(
         "CMO", "Momentum Indicators", "Chande Momentum Oscillator", 0x0A000000,
         List.of(
            new InputInfo(InputType.REAL, "inReal", 0x00000000)
         ),
         List.of(
            new OptInputInfo(
               OptInputType.INTEGER_RANGE, "optInTimePeriod", 0x00000000,
               "Time Period", "Time period", 14.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               2, 100000, 4, 200, 1, null)
         ),
         List.of(
            new OutputInfo(OutputType.REAL, "outReal", 0x00000001)
         ));
   }

   private static FunctionInfo f_CMOU() {
      return new FunctionInfo(
         "CMOU", "Momentum Indicators", "Chande Momentum Oscillator (Unsmoothed)", 0x02000000,
         List.of(
            new InputInfo(InputType.REAL, "inReal", 0x00000000)
         ),
         List.of(
            new OptInputInfo(
               OptInputType.INTEGER_RANGE, "optInTimePeriod", 0x00000000,
               "Time Period", "Time period", 14.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               2, 100000, 4, 200, 1, null)
         ),
         List.of(
            new OutputInfo(OutputType.REAL, "outReal", 0x00000001)
         ));
   }

   private static FunctionInfo f_CORREL() {
      return new FunctionInfo(
         "CORREL", "Statistic Functions", "Pearson's Correlation Coefficient (r)", 0x02000000,
         List.of(
            new InputInfo(InputType.REAL, "inReal0", 0x00000000),
            new InputInfo(InputType.REAL, "inReal1", 0x00000000)
         ),
         List.of(
            new OptInputInfo(
               OptInputType.INTEGER_RANGE, "optInTimePeriod", 0x00000000,
               "Time Period", "Time period", 30.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               1, 100000, 1, 200, 1, null)
         ),
         List.of(
            new OutputInfo(OutputType.REAL, "outReal", 0x00000001)
         ));
   }

   private static FunctionInfo f_COS() {
      return new FunctionInfo(
         "COS", "Math Transform", "Vector Trigonometric Cos", 0x02000000,
         List.of(
            new InputInfo(InputType.REAL, "inReal", 0x00000000)
         ),
         List.of(),
         List.of(
            new OutputInfo(OutputType.REAL, "outReal", 0x00000001)
         ));
   }

   private static FunctionInfo f_COSH() {
      return new FunctionInfo(
         "COSH", "Math Transform", "Vector Trigonometric Cosh", 0x02000000,
         List.of(
            new InputInfo(InputType.REAL, "inReal", 0x00000000)
         ),
         List.of(),
         List.of(
            new OutputInfo(OutputType.REAL, "outReal", 0x00000001)
         ));
   }

   private static FunctionInfo f_DEMA() {
      return new FunctionInfo(
         "DEMA", "Overlap Studies", "Double Exponential Moving Average", 0x03000001,
         List.of(
            new InputInfo(InputType.REAL, "inReal", 0x00000000)
         ),
         List.of(
            new OptInputInfo(
               OptInputType.INTEGER_RANGE, "optInTimePeriod", 0x00000000,
               "Time Period", "Time period", 30.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               1, 100000, 1, 200, 1, null)
         ),
         List.of(
            new OutputInfo(OutputType.REAL, "outReal", 0x00000001)
         ));
   }

   private static FunctionInfo f_DIV() {
      return new FunctionInfo(
         "DIV", "Math Operators", "Vector Arithmetic Div", 0x42000000,
         List.of(
            new InputInfo(InputType.REAL, "inReal0", 0x00000000),
            new InputInfo(InputType.REAL, "inReal1", 0x00000000)
         ),
         List.of(),
         List.of(
            new OutputInfo(OutputType.REAL, "outReal", 0x00000001)
         ));
   }

   private static FunctionInfo f_DX() {
      return new FunctionInfo(
         "DX", "Momentum Indicators", "Directional Movement Index", 0x0A000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceHLC", 0x0000000E)
         ),
         List.of(
            new OptInputInfo(
               OptInputType.INTEGER_RANGE, "optInTimePeriod", 0x00000000,
               "Time Period", "Time period", 14.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               2, 100000, 4, 200, 1, null)
         ),
         List.of(
            new OutputInfo(OutputType.REAL, "outReal", 0x00000001)
         ));
   }

   private static FunctionInfo f_EFI() {
      return new FunctionInfo(
         "EFI", "Volume Indicators", "Elder's Force Index", 0x02000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceCV", 0x00000018)
         ),
         List.of(
            new OptInputInfo(
               OptInputType.INTEGER_RANGE, "optInTimePeriod", 0x00000000,
               "Time Period", "Time period", 13.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               1, 100000, 1, 200, 1, null)
         ),
         List.of(
            new OutputInfo(OutputType.REAL, "outReal", 0x00000001)
         ));
   }

   private static FunctionInfo f_EMA() {
      return new FunctionInfo(
         "EMA", "Overlap Studies", "Exponential Moving Average", 0x0B000001,
         List.of(
            new InputInfo(InputType.REAL, "inReal", 0x00000000)
         ),
         List.of(
            new OptInputInfo(
               OptInputType.INTEGER_RANGE, "optInTimePeriod", 0x00000000,
               "Time Period", "Time period", 30.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               1, 100000, 1, 200, 1, null)
         ),
         List.of(
            new OutputInfo(OutputType.REAL, "outReal", 0x00000001)
         ));
   }

   private static FunctionInfo f_EXP() {
      return new FunctionInfo(
         "EXP", "Math Transform", "Vector Arithmetic Exp", 0x02000000,
         List.of(
            new InputInfo(InputType.REAL, "inReal", 0x00000000)
         ),
         List.of(),
         List.of(
            new OutputInfo(OutputType.REAL, "outReal", 0x00000001)
         ));
   }

   private static FunctionInfo f_FLOOR() {
      return new FunctionInfo(
         "FLOOR", "Math Transform", "Vector Floor", 0x02000000,
         List.of(
            new InputInfo(InputType.REAL, "inReal", 0x00000000)
         ),
         List.of(),
         List.of(
            new OutputInfo(OutputType.REAL, "outReal", 0x00000001)
         ));
   }

   private static FunctionInfo f_HMA() {
      return new FunctionInfo(
         "HMA", "Overlap Studies", "Hull Moving Average", 0x03000001,
         List.of(
            new InputInfo(InputType.REAL, "inReal", 0x00000000)
         ),
         List.of(
            new OptInputInfo(
               OptInputType.INTEGER_RANGE, "optInTimePeriod", 0x00000000,
               "Time Period", "Time period", 20.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               1, 100000, 1, 200, 1, null)
         ),
         List.of(
            new OutputInfo(OutputType.REAL, "outReal", 0x00000001)
         ));
   }

   private static FunctionInfo f_HT_DCPERIOD() {
      return new FunctionInfo(
         "HT_DCPERIOD", "Cycle Indicators", "Hilbert Transform - Dominant Cycle Period", 0x0A000000,
         List.of(
            new InputInfo(InputType.REAL, "inReal", 0x00000000)
         ),
         List.of(),
         List.of(
            new OutputInfo(OutputType.REAL, "outReal", 0x00000001)
         ));
   }

   private static FunctionInfo f_HT_DCPHASE() {
      return new FunctionInfo(
         "HT_DCPHASE", "Cycle Indicators", "Hilbert Transform - Dominant Cycle Phase", 0x0A000000,
         List.of(
            new InputInfo(InputType.REAL, "inReal", 0x00000000)
         ),
         List.of(),
         List.of(
            new OutputInfo(OutputType.REAL, "outReal", 0x00000001)
         ));
   }

   private static FunctionInfo f_HT_PHASOR() {
      return new FunctionInfo(
         "HT_PHASOR", "Cycle Indicators", "Hilbert Transform - Phasor Components", 0x0A000000,
         List.of(
            new InputInfo(InputType.REAL, "inReal", 0x00000000)
         ),
         List.of(),
         List.of(
            new OutputInfo(OutputType.REAL, "outInPhase", 0x00000001),
            new OutputInfo(OutputType.REAL, "outQuadrature", 0x00000004)
         ));
   }

   private static FunctionInfo f_HT_SINE() {
      return new FunctionInfo(
         "HT_SINE", "Cycle Indicators", "Hilbert Transform - SineWave", 0x0A000000,
         List.of(
            new InputInfo(InputType.REAL, "inReal", 0x00000000)
         ),
         List.of(),
         List.of(
            new OutputInfo(OutputType.REAL, "outSine", 0x00000001),
            new OutputInfo(OutputType.REAL, "outLeadSine", 0x00000004)
         ));
   }

   private static FunctionInfo f_HT_TRENDLINE() {
      return new FunctionInfo(
         "HT_TRENDLINE", "Overlap Studies", "Hilbert Transform - Instantaneous Trendline", 0x0B000000,
         List.of(
            new InputInfo(InputType.REAL, "inReal", 0x00000000)
         ),
         List.of(),
         List.of(
            new OutputInfo(OutputType.REAL, "outReal", 0x00000001)
         ));
   }

   private static FunctionInfo f_HT_TRENDMODE() {
      return new FunctionInfo(
         "HT_TRENDMODE", "Cycle Indicators", "Hilbert Transform - Trend vs Cycle Mode", 0x0A000000,
         List.of(
            new InputInfo(InputType.REAL, "inReal", 0x00000000)
         ),
         List.of(),
         List.of(
            new OutputInfo(OutputType.INTEGER, "outInteger", 0x00000001)
         ));
   }

   private static FunctionInfo f_IMI() {
      return new FunctionInfo(
         "IMI", "Momentum Indicators", "Intraday Momentum Index", 0x02000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceOC", 0x00000009)
         ),
         List.of(
            new OptInputInfo(
               OptInputType.INTEGER_RANGE, "optInTimePeriod", 0x00000000,
               "Time Period", "Time period", 14.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               2, 100000, 4, 200, 1, null)
         ),
         List.of(
            new OutputInfo(OutputType.REAL, "outReal", 0x00000001)
         ));
   }

   private static FunctionInfo f_KAMA() {
      return new FunctionInfo(
         "KAMA", "Overlap Studies", "Kaufman Adaptive Moving Average", 0x0B000001,
         List.of(
            new InputInfo(InputType.REAL, "inReal", 0x00000000)
         ),
         List.of(
            new OptInputInfo(
               OptInputType.INTEGER_RANGE, "optInTimePeriod", 0x00000000,
               "Time Period", "Time period", 30.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               1, 100000, 1, 200, 1, null)
         ),
         List.of(
            new OutputInfo(OutputType.REAL, "outReal", 0x00000001)
         ));
   }

   private static FunctionInfo f_LINEARREG() {
      return new FunctionInfo(
         "LINEARREG", "Statistic Functions", "Linear Regression", 0x03000000,
         List.of(
            new InputInfo(InputType.REAL, "inReal", 0x00000000)
         ),
         List.of(
            new OptInputInfo(
               OptInputType.INTEGER_RANGE, "optInTimePeriod", 0x00000000,
               "Time Period", "Time period", 14.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               2, 100000, 4, 200, 1, null)
         ),
         List.of(
            new OutputInfo(OutputType.REAL, "outReal", 0x00000001)
         ));
   }

   private static FunctionInfo f_LINEARREG_ANGLE() {
      return new FunctionInfo(
         "LINEARREG_ANGLE", "Statistic Functions", "Linear Regression Angle", 0x02000000,
         List.of(
            new InputInfo(InputType.REAL, "inReal", 0x00000000)
         ),
         List.of(
            new OptInputInfo(
               OptInputType.INTEGER_RANGE, "optInTimePeriod", 0x00000000,
               "Time Period", "Time period", 14.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               2, 100000, 4, 200, 1, null)
         ),
         List.of(
            new OutputInfo(OutputType.REAL, "outReal", 0x00000001)
         ));
   }

   private static FunctionInfo f_LINEARREG_INTERCEPT() {
      return new FunctionInfo(
         "LINEARREG_INTERCEPT", "Statistic Functions", "Linear Regression Intercept", 0x03000000,
         List.of(
            new InputInfo(InputType.REAL, "inReal", 0x00000000)
         ),
         List.of(
            new OptInputInfo(
               OptInputType.INTEGER_RANGE, "optInTimePeriod", 0x00000000,
               "Time Period", "Time period", 14.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               2, 100000, 4, 200, 1, null)
         ),
         List.of(
            new OutputInfo(OutputType.REAL, "outReal", 0x00000001)
         ));
   }

   private static FunctionInfo f_LINEARREG_SLOPE() {
      return new FunctionInfo(
         "LINEARREG_SLOPE", "Statistic Functions", "Linear Regression Slope", 0x02000000,
         List.of(
            new InputInfo(InputType.REAL, "inReal", 0x00000000)
         ),
         List.of(
            new OptInputInfo(
               OptInputType.INTEGER_RANGE, "optInTimePeriod", 0x00000000,
               "Time Period", "Time period", 14.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               2, 100000, 4, 200, 1, null)
         ),
         List.of(
            new OutputInfo(OutputType.REAL, "outReal", 0x00000001)
         ));
   }

   private static FunctionInfo f_LN() {
      return new FunctionInfo(
         "LN", "Math Transform", "Vector Log Natural", 0x42000000,
         List.of(
            new InputInfo(InputType.REAL, "inReal", 0x00000000)
         ),
         List.of(),
         List.of(
            new OutputInfo(OutputType.REAL, "outReal", 0x00000001)
         ));
   }

   private static FunctionInfo f_LOG10() {
      return new FunctionInfo(
         "LOG10", "Math Transform", "Vector Log10", 0x42000000,
         List.of(
            new InputInfo(InputType.REAL, "inReal", 0x00000000)
         ),
         List.of(),
         List.of(
            new OutputInfo(OutputType.REAL, "outReal", 0x00000001)
         ));
   }

   private static FunctionInfo f_MA() {
      return new FunctionInfo(
         "MA", "Overlap Studies", "Moving average", 0x03000001,
         List.of(
            new InputInfo(InputType.REAL, "inReal", 0x00000000)
         ),
         List.of(
            new OptInputInfo(
               OptInputType.INTEGER_RANGE, "optInTimePeriod", 0x00000000,
               "Time Period", "Time period", 30.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               1, 100000, 1, 200, 1, null),
            new OptInputInfo(
               OptInputType.INTEGER_LIST, "optInMAType", 0x00000000,
               "MA Type", "Type of Moving Average", 0.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               0, 0, 0, 0, 0, "0=SMA;1=EMA;2=WMA;3=DEMA;4=TEMA;5=TRIMA;6=KAMA;7=MAMA;8=T3;9=HMA;10=DISABLED;11=DEFAULT")
         ),
         List.of(
            new OutputInfo(OutputType.REAL, "outReal", 0x00000001)
         ));
   }

   private static FunctionInfo f_MACD() {
      return new FunctionInfo(
         "MACD", "Momentum Indicators", "Moving Average Convergence/Divergence", 0x02000000,
         List.of(
            new InputInfo(InputType.REAL, "inReal", 0x00000000)
         ),
         List.of(
            new OptInputInfo(
               OptInputType.INTEGER_RANGE, "optInFastPeriod", 0x00000000,
               "Fast Period", "Period of the fast MA", 12.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               2, 100000, 4, 200, 1, null),
            new OptInputInfo(
               OptInputType.INTEGER_RANGE, "optInSlowPeriod", 0x00000000,
               "Slow Period", "Period of the slow MA", 26.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               2, 100000, 4, 200, 1, null),
            new OptInputInfo(
               OptInputType.INTEGER_RANGE, "optInSignalPeriod", 0x00000000,
               "Signal Period", "Smoothing for the signal line (period length)", 9.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               1, 100000, 1, 200, 1, null)
         ),
         List.of(
            new OutputInfo(OutputType.REAL, "outMACD", 0x00000001),
            new OutputInfo(OutputType.REAL, "outMACDSignal", 0x00000004),
            new OutputInfo(OutputType.REAL, "outMACDHist", 0x00000010)
         ));
   }

   private static FunctionInfo f_MACDEXT() {
      return new FunctionInfo(
         "MACDEXT", "Momentum Indicators", "MACD with controllable MA type", 0x02000000,
         List.of(
            new InputInfo(InputType.REAL, "inReal", 0x00000000)
         ),
         List.of(
            new OptInputInfo(
               OptInputType.INTEGER_RANGE, "optInFastPeriod", 0x00000000,
               "Fast Period", "Period of the fast MA", 12.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               2, 100000, 4, 200, 1, null),
            new OptInputInfo(
               OptInputType.INTEGER_LIST, "optInFastMAType", 0x00000000,
               "Fast MA", "Type of Moving Average for fast MA", 0.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               0, 0, 0, 0, 0, "0=SMA;1=EMA;2=WMA;3=DEMA;4=TEMA;5=TRIMA;6=KAMA;7=MAMA;8=T3;9=HMA;10=DISABLED;11=DEFAULT"),
            new OptInputInfo(
               OptInputType.INTEGER_RANGE, "optInSlowPeriod", 0x00000000,
               "Slow Period", "Period of the slow MA", 26.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               2, 100000, 4, 200, 1, null),
            new OptInputInfo(
               OptInputType.INTEGER_LIST, "optInSlowMAType", 0x00000000,
               "Slow MA", "Type of Moving Average for slow MA", 0.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               0, 0, 0, 0, 0, "0=SMA;1=EMA;2=WMA;3=DEMA;4=TEMA;5=TRIMA;6=KAMA;7=MAMA;8=T3;9=HMA;10=DISABLED;11=DEFAULT"),
            new OptInputInfo(
               OptInputType.INTEGER_RANGE, "optInSignalPeriod", 0x00000000,
               "Signal Period", "Smoothing for the signal line (period length)", 9.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               1, 100000, 1, 200, 1, null),
            new OptInputInfo(
               OptInputType.INTEGER_LIST, "optInSignalMAType", 0x00000000,
               "Signal MA", "Type of Moving Average for signal line", 0.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               0, 0, 0, 0, 0, "0=SMA;1=EMA;2=WMA;3=DEMA;4=TEMA;5=TRIMA;6=KAMA;7=MAMA;8=T3;9=HMA;10=DISABLED;11=DEFAULT")
         ),
         List.of(
            new OutputInfo(OutputType.REAL, "outMACD", 0x00000001),
            new OutputInfo(OutputType.REAL, "outMACDSignal", 0x00000004),
            new OutputInfo(OutputType.REAL, "outMACDHist", 0x00000010)
         ));
   }

   private static FunctionInfo f_MACDFIX() {
      return new FunctionInfo(
         "MACDFIX", "Momentum Indicators", "Moving Average Convergence/Divergence Fix 12/26", 0x02000000,
         List.of(
            new InputInfo(InputType.REAL, "inReal", 0x00000000)
         ),
         List.of(
            new OptInputInfo(
               OptInputType.INTEGER_RANGE, "optInSignalPeriod", 0x00000000,
               "Signal Period", "Smoothing for the signal line (period length)", 9.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               1, 100000, 1, 200, 1, null)
         ),
         List.of(
            new OutputInfo(OutputType.REAL, "outMACD", 0x00000001),
            new OutputInfo(OutputType.REAL, "outMACDSignal", 0x00000004),
            new OutputInfo(OutputType.REAL, "outMACDHist", 0x00000010)
         ));
   }

   private static FunctionInfo f_MAMA() {
      return new FunctionInfo(
         "MAMA", "Overlap Studies", "MESA Adaptive Moving Average", 0x0B000000,
         List.of(
            new InputInfo(InputType.REAL, "inReal", 0x00000000)
         ),
         List.of(
            new OptInputInfo(
               OptInputType.REAL_RANGE, "optInFastLimit", 0x00000000,
               "Fast Limit", "Upper limit use in the adaptive algorithm", 0.5,
               0.01, 0.99, 2, 0.21, 0.8, 0.01,
               0, 0, 0, 0, 0, null),
            new OptInputInfo(
               OptInputType.REAL_RANGE, "optInSlowLimit", 0x00000000,
               "Slow Limit", "Lower limit use in the adaptive algorithm", 0.05,
               0.01, 0.99, 2, 0.01, 0.6, 0.01,
               0, 0, 0, 0, 0, null)
         ),
         List.of(
            new OutputInfo(OutputType.REAL, "outMAMA", 0x00000001),
            new OutputInfo(OutputType.REAL, "outFAMA", 0x00002004)
         ));
   }

   private static FunctionInfo f_MARKETFI() {
      return new FunctionInfo(
         "MARKETFI", "Volume Indicators", "Market Facilitation Index", 0x02000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceHLV", 0x00000016)
         ),
         List.of(),
         List.of(
            new OutputInfo(OutputType.REAL, "outReal", 0x00000001)
         ));
   }

   private static FunctionInfo f_MAVP() {
      return new FunctionInfo(
         "MAVP", "Overlap Studies", "Moving average with variable period", 0x03000000,
         List.of(
            new InputInfo(InputType.REAL, "inReal", 0x00000000),
            new InputInfo(InputType.REAL, "inPeriods", 0x00000000)
         ),
         List.of(
            new OptInputInfo(
               OptInputType.INTEGER_RANGE, "optInMinPeriod", 0x00000000,
               "Minimum Period", "Value less than minimum will be changed to Minimum period", 2.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               1, 100000, 1, 200, 1, null),
            new OptInputInfo(
               OptInputType.INTEGER_RANGE, "optInMaxPeriod", 0x00000000,
               "Maximum Period", "Value higher than maximum will be changed to Maximum period", 30.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               1, 100000, 1, 200, 1, null),
            new OptInputInfo(
               OptInputType.INTEGER_LIST, "optInMAType", 0x00000000,
               "MA Type", "Type of Moving Average", 0.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               0, 0, 0, 0, 0, "0=SMA;1=EMA;2=WMA;3=DEMA;4=TEMA;5=TRIMA;6=KAMA;7=MAMA;8=T3;9=HMA;10=DISABLED;11=DEFAULT")
         ),
         List.of(
            new OutputInfo(OutputType.REAL, "outReal", 0x00000001)
         ));
   }

   private static FunctionInfo f_MAX() {
      return new FunctionInfo(
         "MAX", "Math Operators", "Highest value over a specified period", 0x03000000,
         List.of(
            new InputInfo(InputType.REAL, "inReal", 0x00000000)
         ),
         List.of(
            new OptInputInfo(
               OptInputType.INTEGER_RANGE, "optInTimePeriod", 0x00000000,
               "Time Period", "Time period", 30.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               2, 100000, 4, 200, 1, null)
         ),
         List.of(
            new OutputInfo(OutputType.REAL, "outReal", 0x00000001)
         ));
   }

   private static FunctionInfo f_MAXINDEX() {
      return new FunctionInfo(
         "MAXINDEX", "Math Operators", "Index of highest value over a specified period", 0x02000000,
         List.of(
            new InputInfo(InputType.REAL, "inReal", 0x00000000)
         ),
         List.of(
            new OptInputInfo(
               OptInputType.INTEGER_RANGE, "optInTimePeriod", 0x00000000,
               "Time Period", "Time period", 30.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               2, 100000, 4, 200, 1, null)
         ),
         List.of(
            new OutputInfo(OutputType.INTEGER, "outInteger", 0x00000001)
         ));
   }

   private static FunctionInfo f_MEDPRICE() {
      return new FunctionInfo(
         "MEDPRICE", "Price Transform", "Median Price", 0x03000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceHL", 0x00000006)
         ),
         List.of(),
         List.of(
            new OutputInfo(OutputType.REAL, "outReal", 0x00000001)
         ));
   }

   private static FunctionInfo f_MFI() {
      return new FunctionInfo(
         "MFI", "Momentum Indicators", "Money Flow Index", 0x02000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceHLCV", 0x0000001E)
         ),
         List.of(
            new OptInputInfo(
               OptInputType.INTEGER_RANGE, "optInTimePeriod", 0x00000000,
               "Time Period", "Time period", 14.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               2, 100000, 4, 200, 1, null)
         ),
         List.of(
            new OutputInfo(OutputType.REAL, "outReal", 0x00000001)
         ));
   }

   private static FunctionInfo f_MIDPOINT() {
      return new FunctionInfo(
         "MIDPOINT", "Overlap Studies", "MidPoint over period", 0x03000000,
         List.of(
            new InputInfo(InputType.REAL, "inReal", 0x00000000)
         ),
         List.of(
            new OptInputInfo(
               OptInputType.INTEGER_RANGE, "optInTimePeriod", 0x00000000,
               "Time Period", "Time period", 14.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               2, 100000, 4, 200, 1, null)
         ),
         List.of(
            new OutputInfo(OutputType.REAL, "outReal", 0x00000001)
         ));
   }

   private static FunctionInfo f_MIDPRICE() {
      return new FunctionInfo(
         "MIDPRICE", "Overlap Studies", "Midpoint Price over period", 0x03000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceHL", 0x00000006)
         ),
         List.of(
            new OptInputInfo(
               OptInputType.INTEGER_RANGE, "optInTimePeriod", 0x00000000,
               "Time Period", "Time period", 14.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               2, 100000, 4, 200, 1, null)
         ),
         List.of(
            new OutputInfo(OutputType.REAL, "outReal", 0x00000001)
         ));
   }

   private static FunctionInfo f_MIN() {
      return new FunctionInfo(
         "MIN", "Math Operators", "Lowest value over a specified period", 0x03000000,
         List.of(
            new InputInfo(InputType.REAL, "inReal", 0x00000000)
         ),
         List.of(
            new OptInputInfo(
               OptInputType.INTEGER_RANGE, "optInTimePeriod", 0x00000000,
               "Time Period", "Time period", 30.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               2, 100000, 4, 200, 1, null)
         ),
         List.of(
            new OutputInfo(OutputType.REAL, "outReal", 0x00000001)
         ));
   }

   private static FunctionInfo f_MININDEX() {
      return new FunctionInfo(
         "MININDEX", "Math Operators", "Index of lowest value over a specified period", 0x02000000,
         List.of(
            new InputInfo(InputType.REAL, "inReal", 0x00000000)
         ),
         List.of(
            new OptInputInfo(
               OptInputType.INTEGER_RANGE, "optInTimePeriod", 0x00000000,
               "Time Period", "Time period", 30.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               2, 100000, 4, 200, 1, null)
         ),
         List.of(
            new OutputInfo(OutputType.INTEGER, "outInteger", 0x00000001)
         ));
   }

   private static FunctionInfo f_MINMAX() {
      return new FunctionInfo(
         "MINMAX", "Math Operators", "Lowest and highest values over a specified period", 0x03000000,
         List.of(
            new InputInfo(InputType.REAL, "inReal", 0x00000000)
         ),
         List.of(
            new OptInputInfo(
               OptInputType.INTEGER_RANGE, "optInTimePeriod", 0x00000000,
               "Time Period", "Time period", 30.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               2, 100000, 4, 200, 1, null)
         ),
         List.of(
            new OutputInfo(OutputType.REAL, "outMin", 0x00000001),
            new OutputInfo(OutputType.REAL, "outMax", 0x00000001)
         ));
   }

   private static FunctionInfo f_MINMAXINDEX() {
      return new FunctionInfo(
         "MINMAXINDEX", "Math Operators", "Indexes of lowest and highest values over a specified period", 0x02000000,
         List.of(
            new InputInfo(InputType.REAL, "inReal", 0x00000000)
         ),
         List.of(
            new OptInputInfo(
               OptInputType.INTEGER_RANGE, "optInTimePeriod", 0x00000000,
               "Time Period", "Time period", 30.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               2, 100000, 4, 200, 1, null)
         ),
         List.of(
            new OutputInfo(OutputType.INTEGER, "outMinIdx", 0x00000001),
            new OutputInfo(OutputType.INTEGER, "outMaxIdx", 0x00000001)
         ));
   }

   private static FunctionInfo f_MINUS_DI() {
      return new FunctionInfo(
         "MINUS_DI", "Momentum Indicators", "Minus Directional Indicator", 0x0A000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceHLC", 0x0000000E)
         ),
         List.of(
            new OptInputInfo(
               OptInputType.INTEGER_RANGE, "optInTimePeriod", 0x00000000,
               "Time Period", "Time period", 14.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               1, 100000, 1, 200, 1, null)
         ),
         List.of(
            new OutputInfo(OutputType.REAL, "outReal", 0x00000001)
         ));
   }

   private static FunctionInfo f_MINUS_DM() {
      return new FunctionInfo(
         "MINUS_DM", "Momentum Indicators", "Minus Directional Movement", 0x0A000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceHL", 0x00000006)
         ),
         List.of(
            new OptInputInfo(
               OptInputType.INTEGER_RANGE, "optInTimePeriod", 0x00000000,
               "Time Period", "Time period", 14.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               1, 100000, 1, 200, 1, null)
         ),
         List.of(
            new OutputInfo(OutputType.REAL, "outReal", 0x00000001)
         ));
   }

   private static FunctionInfo f_MOM() {
      return new FunctionInfo(
         "MOM", "Momentum Indicators", "Momentum", 0x02000000,
         List.of(
            new InputInfo(InputType.REAL, "inReal", 0x00000000)
         ),
         List.of(
            new OptInputInfo(
               OptInputType.INTEGER_RANGE, "optInTimePeriod", 0x00000000,
               "Time Period", "Time period", 10.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               1, 100000, 1, 200, 1, null)
         ),
         List.of(
            new OutputInfo(OutputType.REAL, "outReal", 0x00000001)
         ));
   }

   private static FunctionInfo f_MULT() {
      return new FunctionInfo(
         "MULT", "Math Operators", "Vector Arithmetic Mult", 0x02000000,
         List.of(
            new InputInfo(InputType.REAL, "inReal0", 0x00000000),
            new InputInfo(InputType.REAL, "inReal1", 0x00000000)
         ),
         List.of(),
         List.of(
            new OutputInfo(OutputType.REAL, "outReal", 0x00000001)
         ));
   }

   private static FunctionInfo f_NATR() {
      return new FunctionInfo(
         "NATR", "Volatility Indicators", "Normalized Average True Range", 0x0A000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceHLC", 0x0000000E)
         ),
         List.of(
            new OptInputInfo(
               OptInputType.INTEGER_RANGE, "optInTimePeriod", 0x00000000,
               "Time Period", "Time period", 14.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               1, 100000, 1, 200, 1, null)
         ),
         List.of(
            new OutputInfo(OutputType.REAL, "outReal", 0x00000001)
         ));
   }

   private static FunctionInfo f_NVI() {
      return new FunctionInfo(
         "NVI", "Volume Indicators", "Negative Volume Index", 0x22000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceCV", 0x00000018)
         ),
         List.of(),
         List.of(
            new OutputInfo(OutputType.REAL, "outReal", 0x00000001)
         ));
   }

   private static FunctionInfo f_OBV() {
      return new FunctionInfo(
         "OBV", "Volume Indicators", "On Balance Volume", 0x22000000,
         List.of(
            new InputInfo(InputType.REAL, "inReal", 0x00000000),
            new InputInfo(InputType.PRICE, "inPriceV", 0x00000010)
         ),
         List.of(),
         List.of(
            new OutputInfo(OutputType.REAL, "outReal", 0x00000001)
         ));
   }

   private static FunctionInfo f_PLUS_DI() {
      return new FunctionInfo(
         "PLUS_DI", "Momentum Indicators", "Plus Directional Indicator", 0x0A000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceHLC", 0x0000000E)
         ),
         List.of(
            new OptInputInfo(
               OptInputType.INTEGER_RANGE, "optInTimePeriod", 0x00000000,
               "Time Period", "Time period", 14.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               1, 100000, 1, 200, 1, null)
         ),
         List.of(
            new OutputInfo(OutputType.REAL, "outReal", 0x00000001)
         ));
   }

   private static FunctionInfo f_PLUS_DM() {
      return new FunctionInfo(
         "PLUS_DM", "Momentum Indicators", "Plus Directional Movement", 0x0A000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceHL", 0x00000006)
         ),
         List.of(
            new OptInputInfo(
               OptInputType.INTEGER_RANGE, "optInTimePeriod", 0x00000000,
               "Time Period", "Time period", 14.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               1, 100000, 1, 200, 1, null)
         ),
         List.of(
            new OutputInfo(OutputType.REAL, "outReal", 0x00000001)
         ));
   }

   private static FunctionInfo f_PPO() {
      return new FunctionInfo(
         "PPO", "Momentum Indicators", "Percentage Price Oscillator", 0x02000000,
         List.of(
            new InputInfo(InputType.REAL, "inReal", 0x00000000)
         ),
         List.of(
            new OptInputInfo(
               OptInputType.INTEGER_RANGE, "optInFastPeriod", 0x00000000,
               "Fast Period", "Period of the fast MA", 12.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               2, 100000, 4, 200, 1, null),
            new OptInputInfo(
               OptInputType.INTEGER_RANGE, "optInSlowPeriod", 0x00000000,
               "Slow Period", "Period of the slow MA", 26.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               2, 100000, 4, 200, 1, null),
            new OptInputInfo(
               OptInputType.INTEGER_LIST, "optInMAType", 0x00000000,
               "MA Type", "Type of Moving Average", 1.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               0, 0, 0, 0, 0, "0=SMA;1=EMA;2=WMA;3=DEMA;4=TEMA;5=TRIMA;6=KAMA;7=MAMA;8=T3;9=HMA;10=DISABLED;11=DEFAULT")
         ),
         List.of(
            new OutputInfo(OutputType.REAL, "outReal", 0x00000001)
         ));
   }

   private static FunctionInfo f_PVI() {
      return new FunctionInfo(
         "PVI", "Volume Indicators", "Positive Volume Index", 0x22000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceCV", 0x00000018)
         ),
         List.of(),
         List.of(
            new OutputInfo(OutputType.REAL, "outReal", 0x00000001)
         ));
   }

   private static FunctionInfo f_PVO() {
      return new FunctionInfo(
         "PVO", "Volume Indicators", "Percentage Volume Oscillator", 0x02000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceV", 0x00000010)
         ),
         List.of(
            new OptInputInfo(
               OptInputType.INTEGER_RANGE, "optInFastPeriod", 0x00000000,
               "Fast Period", "Period of the fast MA", 12.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               2, 100000, 4, 200, 1, null),
            new OptInputInfo(
               OptInputType.INTEGER_RANGE, "optInSlowPeriod", 0x00000000,
               "Slow Period", "Period of the slow MA", 26.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               2, 100000, 4, 200, 1, null),
            new OptInputInfo(
               OptInputType.INTEGER_LIST, "optInMAType", 0x00000000,
               "MA Type", "Type of Moving Average", 1.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               0, 0, 0, 0, 0, "0=SMA;1=EMA;2=WMA;3=DEMA;4=TEMA;5=TRIMA;6=KAMA;7=MAMA;8=T3;9=HMA;10=DISABLED;11=DEFAULT")
         ),
         List.of(
            new OutputInfo(OutputType.REAL, "outReal", 0x00000001)
         ));
   }

   private static FunctionInfo f_QSTICK() {
      return new FunctionInfo(
         "QSTICK", "Momentum Indicators", "Qstick", 0x02000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceOC", 0x00000009)
         ),
         List.of(
            new OptInputInfo(
               OptInputType.INTEGER_RANGE, "optInTimePeriod", 0x00000000,
               "Time Period", "Time period", 10.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               1, 100000, 4, 200, 1, null)
         ),
         List.of(
            new OutputInfo(OutputType.REAL, "outReal", 0x00000001)
         ));
   }

   private static FunctionInfo f_ROC() {
      return new FunctionInfo(
         "ROC", "Momentum Indicators", "Rate of change : ((price/prevPrice)-1)*100", 0x02000000,
         List.of(
            new InputInfo(InputType.REAL, "inReal", 0x00000000)
         ),
         List.of(
            new OptInputInfo(
               OptInputType.INTEGER_RANGE, "optInTimePeriod", 0x00000000,
               "Time Period", "Time period", 10.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               1, 100000, 1, 200, 1, null)
         ),
         List.of(
            new OutputInfo(OutputType.REAL, "outReal", 0x00000001)
         ));
   }

   private static FunctionInfo f_ROCP() {
      return new FunctionInfo(
         "ROCP", "Momentum Indicators", "Rate of change Percentage: (price-prevPrice)/prevPrice", 0x02000000,
         List.of(
            new InputInfo(InputType.REAL, "inReal", 0x00000000)
         ),
         List.of(
            new OptInputInfo(
               OptInputType.INTEGER_RANGE, "optInTimePeriod", 0x00000000,
               "Time Period", "Time period", 10.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               1, 100000, 1, 200, 1, null)
         ),
         List.of(
            new OutputInfo(OutputType.REAL, "outReal", 0x00000001)
         ));
   }

   private static FunctionInfo f_ROCR() {
      return new FunctionInfo(
         "ROCR", "Momentum Indicators", "Rate of change ratio: (price/prevPrice)", 0x02000000,
         List.of(
            new InputInfo(InputType.REAL, "inReal", 0x00000000)
         ),
         List.of(
            new OptInputInfo(
               OptInputType.INTEGER_RANGE, "optInTimePeriod", 0x00000000,
               "Time Period", "Time period", 10.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               1, 100000, 1, 200, 1, null)
         ),
         List.of(
            new OutputInfo(OutputType.REAL, "outReal", 0x00000001)
         ));
   }

   private static FunctionInfo f_ROCR100() {
      return new FunctionInfo(
         "ROCR100", "Momentum Indicators", "Rate of change ratio 100 scale: (price/prevPrice)*100", 0x02000000,
         List.of(
            new InputInfo(InputType.REAL, "inReal", 0x00000000)
         ),
         List.of(
            new OptInputInfo(
               OptInputType.INTEGER_RANGE, "optInTimePeriod", 0x00000000,
               "Time Period", "Time period", 10.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               1, 100000, 1, 200, 1, null)
         ),
         List.of(
            new OutputInfo(OutputType.REAL, "outReal", 0x00000001)
         ));
   }

   private static FunctionInfo f_RSI() {
      return new FunctionInfo(
         "RSI", "Momentum Indicators", "Relative Strength Index", 0x0A000000,
         List.of(
            new InputInfo(InputType.REAL, "inReal", 0x00000000)
         ),
         List.of(
            new OptInputInfo(
               OptInputType.INTEGER_RANGE, "optInTimePeriod", 0x00000000,
               "Time Period", "Time period", 14.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               2, 100000, 4, 200, 1, null)
         ),
         List.of(
            new OutputInfo(OutputType.REAL, "outReal", 0x00000001)
         ));
   }

   private static FunctionInfo f_SAR() {
      return new FunctionInfo(
         "SAR", "Overlap Studies", "Parabolic SAR", 0x23000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceHL", 0x00000006)
         ),
         List.of(
            new OptInputInfo(
               OptInputType.REAL_RANGE, "optInAcceleration", 0x00000000,
               "Acceleration Factor", "Acceleration Factor used up to the Maximum value", 0.02,
               0.0, 3e37, 4, 0.01, 0.2, 0.01,
               0, 0, 0, 0, 0, null),
            new OptInputInfo(
               OptInputType.REAL_RANGE, "optInMaximum", 0x00000000,
               "AF Maximum", "Acceleration Factor Maximum value", 0.2,
               0.0, 3e37, 4, 0.2, 0.4, 0.01,
               0, 0, 0, 0, 0, null)
         ),
         List.of(
            new OutputInfo(OutputType.REAL, "outReal", 0x00000001)
         ));
   }

   private static FunctionInfo f_SAREXT() {
      return new FunctionInfo(
         "SAREXT", "Overlap Studies", "Parabolic SAR - Extended", 0x23000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceHL", 0x00000006)
         ),
         List.of(
            new OptInputInfo(
               OptInputType.REAL_RANGE, "optInStartValue", 0x00000000,
               "Start Value", "Start value and direction. 0 for Auto, >0 for Long, <0 for Short", 0.0,
               -3e37, 3e37, 4, 0.0, 0.0, 0.0,
               0, 0, 0, 0, 0, null),
            new OptInputInfo(
               OptInputType.REAL_RANGE, "optInOffsetOnReverse", 0x00000000,
               "Offset on Reverse", "Percent offset added/removed to initial stop on short/long reversal", 0.0,
               0.0, 3e37, 4, 0.01, 0.15, 0.01,
               0, 0, 0, 0, 0, null),
            new OptInputInfo(
               OptInputType.REAL_RANGE, "optInAccelerationInitLong", 0x00000000,
               "AF Init Long", "Acceleration Factor initial value for the Long direction", 0.02,
               0.0, 3e37, 4, 0.01, 0.19, 0.01,
               0, 0, 0, 0, 0, null),
            new OptInputInfo(
               OptInputType.REAL_RANGE, "optInAccelerationLong", 0x00000000,
               "AF Long", "Acceleration Factor for the Long direction", 0.02,
               0.0, 3e37, 4, 0.01, 0.2, 0.01,
               0, 0, 0, 0, 0, null),
            new OptInputInfo(
               OptInputType.REAL_RANGE, "optInAccelerationMaxLong", 0x00000000,
               "AF Max Long", "Acceleration Factor maximum value for the Long direction", 0.2,
               0.0, 3e37, 4, 0.2, 0.4, 0.01,
               0, 0, 0, 0, 0, null),
            new OptInputInfo(
               OptInputType.REAL_RANGE, "optInAccelerationInitShort", 0x00000000,
               "AF Init Short", "Acceleration Factor initial value for the Short direction", 0.02,
               0.0, 3e37, 4, 0.01, 0.19, 0.01,
               0, 0, 0, 0, 0, null),
            new OptInputInfo(
               OptInputType.REAL_RANGE, "optInAccelerationShort", 0x00000000,
               "AF Short", "Acceleration Factor for the Short direction", 0.02,
               0.0, 3e37, 4, 0.01, 0.2, 0.01,
               0, 0, 0, 0, 0, null),
            new OptInputInfo(
               OptInputType.REAL_RANGE, "optInAccelerationMaxShort", 0x00000000,
               "AF Max Short", "Acceleration Factor maximum value for the Short direction", 0.2,
               0.0, 3e37, 4, 0.2, 0.4, 0.01,
               0, 0, 0, 0, 0, null)
         ),
         List.of(
            new OutputInfo(OutputType.REAL, "outReal", 0x00000001)
         ));
   }

   private static FunctionInfo f_SIN() {
      return new FunctionInfo(
         "SIN", "Math Transform", "Vector Trigonometric Sin", 0x02000000,
         List.of(
            new InputInfo(InputType.REAL, "inReal", 0x00000000)
         ),
         List.of(),
         List.of(
            new OutputInfo(OutputType.REAL, "outReal", 0x00000001)
         ));
   }

   private static FunctionInfo f_SINH() {
      return new FunctionInfo(
         "SINH", "Math Transform", "Vector Trigonometric Sinh", 0x02000000,
         List.of(
            new InputInfo(InputType.REAL, "inReal", 0x00000000)
         ),
         List.of(),
         List.of(
            new OutputInfo(OutputType.REAL, "outReal", 0x00000001)
         ));
   }

   private static FunctionInfo f_SMA() {
      return new FunctionInfo(
         "SMA", "Overlap Studies", "Simple Moving Average", 0x03000001,
         List.of(
            new InputInfo(InputType.REAL, "inReal", 0x00000000)
         ),
         List.of(
            new OptInputInfo(
               OptInputType.INTEGER_RANGE, "optInTimePeriod", 0x00000000,
               "Time Period", "Time period", 30.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               1, 100000, 1, 200, 1, null)
         ),
         List.of(
            new OutputInfo(OutputType.REAL, "outReal", 0x00000001)
         ));
   }

   private static FunctionInfo f_SMI() {
      return new FunctionInfo(
         "SMI", "Momentum Indicators", "Stochastic Momentum Index", 0x02000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceHLC", 0x0000000E)
         ),
         List.of(
            new OptInputInfo(
               OptInputType.INTEGER_RANGE, "optInTimePeriod", 0x00000000,
               "Time Period", "Period of the high/low range", 13.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               2, 100000, 4, 200, 1, null),
            new OptInputInfo(
               OptInputType.INTEGER_RANGE, "optInFastPeriod", 0x00000000,
               "Fast Period", "Period of the second smoothing, applied to the first", 2.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               2, 100000, 2, 200, 1, null),
            new OptInputInfo(
               OptInputType.INTEGER_RANGE, "optInSlowPeriod", 0x00000000,
               "Slow Period", "Period of the first smoothing, applied to the raw momentum", 25.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               2, 100000, 2, 200, 1, null),
            new OptInputInfo(
               OptInputType.INTEGER_RANGE, "optInSignalPeriod", 0x00000000,
               "Signal Period", "Smoothing for the signal line (period length)", 9.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               2, 100000, 2, 200, 1, null)
         ),
         List.of(
            new OutputInfo(OutputType.REAL, "outSMI", 0x00000001),
            new OutputInfo(OutputType.REAL, "outSMISignal", 0x00000004)
         ));
   }

   private static FunctionInfo f_SQRT() {
      return new FunctionInfo(
         "SQRT", "Math Transform", "Vector Square Root", 0x42000000,
         List.of(
            new InputInfo(InputType.REAL, "inReal", 0x00000000)
         ),
         List.of(),
         List.of(
            new OutputInfo(OutputType.REAL, "outReal", 0x00000001)
         ));
   }

   private static FunctionInfo f_STDDEV() {
      return new FunctionInfo(
         "STDDEV", "Statistic Functions", "Standard Deviation", 0x02000000,
         List.of(
            new InputInfo(InputType.REAL, "inReal", 0x00000000)
         ),
         List.of(
            new OptInputInfo(
               OptInputType.INTEGER_RANGE, "optInTimePeriod", 0x00000000,
               "Time Period", "Time period", 5.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               2, 100000, 4, 200, 1, null),
            new OptInputInfo(
               OptInputType.REAL_RANGE, "optInNbDev", 0x00000000,
               "Deviations", "Nb of deviations", 1.0,
               -3e37, 3e37, 2, -2.0, 2.0, 0.2,
               0, 0, 0, 0, 0, null)
         ),
         List.of(
            new OutputInfo(OutputType.REAL, "outReal", 0x00000001)
         ));
   }

   private static FunctionInfo f_STOCH() {
      return new FunctionInfo(
         "STOCH", "Momentum Indicators", "Stochastic", 0x02000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceHLC", 0x0000000E)
         ),
         List.of(
            new OptInputInfo(
               OptInputType.INTEGER_RANGE, "optInFastK_Period", 0x00000000,
               "Fast-K Period", "Time period for building the Fast-K line", 5.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               1, 100000, 1, 200, 1, null),
            new OptInputInfo(
               OptInputType.INTEGER_RANGE, "optInSlowK_Period", 0x00000000,
               "Slow-K Period", "Smoothing for making the Slow-K line. Usually set to 3", 3.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               1, 100000, 1, 200, 1, null),
            new OptInputInfo(
               OptInputType.INTEGER_LIST, "optInSlowK_MAType", 0x00000000,
               "Slow-K MA", "Type of Moving Average for Slow-K", 0.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               0, 0, 0, 0, 0, "0=SMA;1=EMA;2=WMA;3=DEMA;4=TEMA;5=TRIMA;6=KAMA;7=MAMA;8=T3;9=HMA;10=DISABLED;11=DEFAULT"),
            new OptInputInfo(
               OptInputType.INTEGER_RANGE, "optInSlowD_Period", 0x00000000,
               "Slow-D Period", "Smoothing for making the Slow-D line", 3.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               1, 100000, 1, 200, 1, null),
            new OptInputInfo(
               OptInputType.INTEGER_LIST, "optInSlowD_MAType", 0x00000000,
               "Slow-D MA", "Type of Moving Average for Slow-D", 0.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               0, 0, 0, 0, 0, "0=SMA;1=EMA;2=WMA;3=DEMA;4=TEMA;5=TRIMA;6=KAMA;7=MAMA;8=T3;9=HMA;10=DISABLED;11=DEFAULT")
         ),
         List.of(
            new OutputInfo(OutputType.REAL, "outSlowK", 0x00000004),
            new OutputInfo(OutputType.REAL, "outSlowD", 0x00000004)
         ));
   }

   private static FunctionInfo f_STOCHF() {
      return new FunctionInfo(
         "STOCHF", "Momentum Indicators", "Stochastic Fast", 0x02000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceHLC", 0x0000000E)
         ),
         List.of(
            new OptInputInfo(
               OptInputType.INTEGER_RANGE, "optInFastK_Period", 0x00000000,
               "Fast-K Period", "Time period for building the Fast-K line", 5.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               1, 100000, 1, 200, 1, null),
            new OptInputInfo(
               OptInputType.INTEGER_RANGE, "optInFastD_Period", 0x00000000,
               "Fast-D Period", "Smoothing for making the Fast-D line. Usually set to 3", 3.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               1, 100000, 1, 200, 1, null),
            new OptInputInfo(
               OptInputType.INTEGER_LIST, "optInFastD_MAType", 0x00000000,
               "Fast-D MA", "Type of Moving Average for Fast-D", 0.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               0, 0, 0, 0, 0, "0=SMA;1=EMA;2=WMA;3=DEMA;4=TEMA;5=TRIMA;6=KAMA;7=MAMA;8=T3;9=HMA;10=DISABLED;11=DEFAULT")
         ),
         List.of(
            new OutputInfo(OutputType.REAL, "outFastK", 0x00000001),
            new OutputInfo(OutputType.REAL, "outFastD", 0x00000001)
         ));
   }

   private static FunctionInfo f_STOCHRSI() {
      return new FunctionInfo(
         "STOCHRSI", "Momentum Indicators", "Stochastic Relative Strength Index", 0x02000000,
         List.of(
            new InputInfo(InputType.REAL, "inReal", 0x00000000)
         ),
         List.of(
            new OptInputInfo(
               OptInputType.INTEGER_RANGE, "optInTimePeriod", 0x00000000,
               "Time Period", "Time period", 14.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               2, 100000, 4, 200, 1, null),
            new OptInputInfo(
               OptInputType.INTEGER_RANGE, "optInFastK_Period", 0x00000000,
               "Fast-K Period", "Time period for building the Fast-K line", 5.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               1, 100000, 1, 200, 1, null),
            new OptInputInfo(
               OptInputType.INTEGER_RANGE, "optInFastD_Period", 0x00000000,
               "Fast-D Period", "Smoothing for making the Fast-D line. Usually set to 3", 3.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               1, 100000, 1, 200, 1, null),
            new OptInputInfo(
               OptInputType.INTEGER_LIST, "optInFastD_MAType", 0x00000000,
               "Fast-D MA", "Type of Moving Average for Fast-D", 0.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               0, 0, 0, 0, 0, "0=SMA;1=EMA;2=WMA;3=DEMA;4=TEMA;5=TRIMA;6=KAMA;7=MAMA;8=T3;9=HMA;10=DISABLED;11=DEFAULT")
         ),
         List.of(
            new OutputInfo(OutputType.REAL, "outFastK", 0x00000001),
            new OutputInfo(OutputType.REAL, "outFastD", 0x00000001)
         ));
   }

   private static FunctionInfo f_SUB() {
      return new FunctionInfo(
         "SUB", "Math Operators", "Vector Arithmetic Subtraction", 0x02000000,
         List.of(
            new InputInfo(InputType.REAL, "inReal0", 0x00000000),
            new InputInfo(InputType.REAL, "inReal1", 0x00000000)
         ),
         List.of(),
         List.of(
            new OutputInfo(OutputType.REAL, "outReal", 0x00000001)
         ));
   }

   private static FunctionInfo f_SUM() {
      return new FunctionInfo(
         "SUM", "Math Operators", "Summation", 0x02000000,
         List.of(
            new InputInfo(InputType.REAL, "inReal", 0x00000000)
         ),
         List.of(
            new OptInputInfo(
               OptInputType.INTEGER_RANGE, "optInTimePeriod", 0x00000000,
               "Time Period", "Time period", 30.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               2, 100000, 4, 200, 1, null)
         ),
         List.of(
            new OutputInfo(OutputType.REAL, "outReal", 0x00000001)
         ));
   }

   private static FunctionInfo f_T3() {
      return new FunctionInfo(
         "T3", "Overlap Studies", "Triple Exponential Moving Average (T3)", 0x0B000001,
         List.of(
            new InputInfo(InputType.REAL, "inReal", 0x00000000)
         ),
         List.of(
            new OptInputInfo(
               OptInputType.INTEGER_RANGE, "optInTimePeriod", 0x00000000,
               "Time Period", "Time period", 5.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               1, 100000, 1, 200, 1, null),
            new OptInputInfo(
               OptInputType.REAL_RANGE, "optInVFactor", 0x00000000,
               "Volume Factor", "Volume Factor", 0.7,
               0.0, 1.0, 2, 0.01, 1.0, 0.05,
               0, 0, 0, 0, 0, null)
         ),
         List.of(
            new OutputInfo(OutputType.REAL, "outReal", 0x00000001)
         ));
   }

   private static FunctionInfo f_TAN() {
      return new FunctionInfo(
         "TAN", "Math Transform", "Vector Trigonometric Tan", 0x02000000,
         List.of(
            new InputInfo(InputType.REAL, "inReal", 0x00000000)
         ),
         List.of(),
         List.of(
            new OutputInfo(OutputType.REAL, "outReal", 0x00000001)
         ));
   }

   private static FunctionInfo f_TANH() {
      return new FunctionInfo(
         "TANH", "Math Transform", "Vector Trigonometric Tanh", 0x02000000,
         List.of(
            new InputInfo(InputType.REAL, "inReal", 0x00000000)
         ),
         List.of(),
         List.of(
            new OutputInfo(OutputType.REAL, "outReal", 0x00000001)
         ));
   }

   private static FunctionInfo f_TEMA() {
      return new FunctionInfo(
         "TEMA", "Overlap Studies", "Triple Exponential Moving Average", 0x03000001,
         List.of(
            new InputInfo(InputType.REAL, "inReal", 0x00000000)
         ),
         List.of(
            new OptInputInfo(
               OptInputType.INTEGER_RANGE, "optInTimePeriod", 0x00000000,
               "Time Period", "Time period", 30.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               1, 100000, 1, 200, 1, null)
         ),
         List.of(
            new OutputInfo(OutputType.REAL, "outReal", 0x00000001)
         ));
   }

   private static FunctionInfo f_TRANGE() {
      return new FunctionInfo(
         "TRANGE", "Volatility Indicators", "True Range", 0x02000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceHLC", 0x0000000E)
         ),
         List.of(),
         List.of(
            new OutputInfo(OutputType.REAL, "outReal", 0x00000001)
         ));
   }

   private static FunctionInfo f_TRIMA() {
      return new FunctionInfo(
         "TRIMA", "Overlap Studies", "Triangular Moving Average", 0x03000001,
         List.of(
            new InputInfo(InputType.REAL, "inReal", 0x00000000)
         ),
         List.of(
            new OptInputInfo(
               OptInputType.INTEGER_RANGE, "optInTimePeriod", 0x00000000,
               "Time Period", "Time period", 30.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               1, 100000, 1, 200, 1, null)
         ),
         List.of(
            new OutputInfo(OutputType.REAL, "outReal", 0x00000001)
         ));
   }

   private static FunctionInfo f_TRIX() {
      return new FunctionInfo(
         "TRIX", "Momentum Indicators", "1-day Rate-Of-Change (ROC) of a Triple Smooth EMA", 0x02000000,
         List.of(
            new InputInfo(InputType.REAL, "inReal", 0x00000000)
         ),
         List.of(
            new OptInputInfo(
               OptInputType.INTEGER_RANGE, "optInTimePeriod", 0x00000000,
               "Time Period", "Time period", 30.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               1, 100000, 1, 200, 1, null)
         ),
         List.of(
            new OutputInfo(OutputType.REAL, "outReal", 0x00000001)
         ));
   }

   private static FunctionInfo f_TSF() {
      return new FunctionInfo(
         "TSF", "Statistic Functions", "Time Series Forecast", 0x03000000,
         List.of(
            new InputInfo(InputType.REAL, "inReal", 0x00000000)
         ),
         List.of(
            new OptInputInfo(
               OptInputType.INTEGER_RANGE, "optInTimePeriod", 0x00000000,
               "Time Period", "Time period", 14.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               2, 100000, 4, 200, 1, null)
         ),
         List.of(
            new OutputInfo(OutputType.REAL, "outReal", 0x00000001)
         ));
   }

   private static FunctionInfo f_TYPPRICE() {
      return new FunctionInfo(
         "TYPPRICE", "Price Transform", "Typical Price", 0x03000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceHLC", 0x0000000E)
         ),
         List.of(),
         List.of(
            new OutputInfo(OutputType.REAL, "outReal", 0x00000001)
         ));
   }

   private static FunctionInfo f_ULTOSC() {
      return new FunctionInfo(
         "ULTOSC", "Momentum Indicators", "Ultimate Oscillator", 0x02000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceHLC", 0x0000000E)
         ),
         List.of(
            new OptInputInfo(
               OptInputType.INTEGER_RANGE, "optInTimePeriod1", 0x00000000,
               "First Period", "Number of bars for 1st period.", 7.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               1, 100000, 1, 200, 1, null),
            new OptInputInfo(
               OptInputType.INTEGER_RANGE, "optInTimePeriod2", 0x00000000,
               "Second Period", "Number of bars fro 2nd period", 14.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               1, 100000, 1, 200, 1, null),
            new OptInputInfo(
               OptInputType.INTEGER_RANGE, "optInTimePeriod3", 0x00000000,
               "Third Period", "Number of bars for 3rd period", 28.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               1, 100000, 1, 200, 1, null)
         ),
         List.of(
            new OutputInfo(OutputType.REAL, "outReal", 0x00000001)
         ));
   }

   private static FunctionInfo f_VAR() {
      return new FunctionInfo(
         "VAR", "Statistic Functions", "Variance", 0x02000000,
         List.of(
            new InputInfo(InputType.REAL, "inReal", 0x00000000)
         ),
         List.of(
            new OptInputInfo(
               OptInputType.INTEGER_RANGE, "optInTimePeriod", 0x00000000,
               "Time Period", "Time period", 5.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               1, 100000, 1, 200, 1, null),
            new OptInputInfo(
               OptInputType.REAL_RANGE, "optInNbDev", 0x00000000,
               "Deviations", "Nb of deviations", 1.0,
               -3e37, 3e37, 2, -2.0, 2.0, 0.2,
               0, 0, 0, 0, 0, null)
         ),
         List.of(
            new OutputInfo(OutputType.REAL, "outReal", 0x00000001)
         ));
   }

   private static FunctionInfo f_VWMA() {
      return new FunctionInfo(
         "VWMA", "Overlap Studies", "Volume Weighted Moving Average", 0x43000001,
         List.of(
            new InputInfo(InputType.REAL, "inReal", 0x00000000),
            new InputInfo(InputType.PRICE, "inPriceV", 0x00000010)
         ),
         List.of(
            new OptInputInfo(
               OptInputType.INTEGER_RANGE, "optInTimePeriod", 0x00000000,
               "Time Period", "Time period", 30.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               1, 100000, 1, 200, 1, null)
         ),
         List.of(
            new OutputInfo(OutputType.REAL, "outReal", 0x00000001)
         ));
   }

   private static FunctionInfo f_WAD() {
      return new FunctionInfo(
         "WAD", "Volume Indicators", "Williams' Accumulation/Distribution (no volume)", 0x22000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceHLC", 0x0000000E)
         ),
         List.of(),
         List.of(
            new OutputInfo(OutputType.REAL, "outReal", 0x00000001)
         ));
   }

   private static FunctionInfo f_WCLPRICE() {
      return new FunctionInfo(
         "WCLPRICE", "Price Transform", "Weighted Close Price", 0x03000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceHLC", 0x0000000E)
         ),
         List.of(),
         List.of(
            new OutputInfo(OutputType.REAL, "outReal", 0x00000001)
         ));
   }

   private static FunctionInfo f_WILLR() {
      return new FunctionInfo(
         "WILLR", "Momentum Indicators", "Williams' %R", 0x02000000,
         List.of(
            new InputInfo(InputType.PRICE, "inPriceHLC", 0x0000000E)
         ),
         List.of(
            new OptInputInfo(
               OptInputType.INTEGER_RANGE, "optInTimePeriod", 0x00000000,
               "Time Period", "Time period", 14.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               2, 100000, 4, 200, 1, null)
         ),
         List.of(
            new OutputInfo(OutputType.REAL, "outReal", 0x00000001)
         ));
   }

   private static FunctionInfo f_WMA() {
      return new FunctionInfo(
         "WMA", "Overlap Studies", "Weighted Moving Average", 0x03000001,
         List.of(
            new InputInfo(InputType.REAL, "inReal", 0x00000000)
         ),
         List.of(
            new OptInputInfo(
               OptInputType.INTEGER_RANGE, "optInTimePeriod", 0x00000000,
               "Time Period", "Time period", 30.0,
               0.0, 0.0, 0, 0.0, 0.0, 0.0,
               1, 100000, 1, 200, 1, null)
         ),
         List.of(
            new OutputInfo(OutputType.REAL, "outReal", 0x00000001)
         ));
   }

}
