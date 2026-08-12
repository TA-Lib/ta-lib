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

import io.github.talib.Core;
import io.github.talib.OutRange;

/**
 * Routes a {@link ParamHolder} onto the typed method it names.
 *
 * <p>A generated {@code switch}, not reflection: the argument lists below are
 * emitted from the same definitions as the methods they call, so a signature
 * change breaks this file at compile time instead of at run time. It also
 * leaves the library AOT- and jlink-friendly.
 */
final class Dispatch {

   private Dispatch() { }

   static OutRange call(ParamHolder h, int startIdx, int endIdx) {
      Core core = h.core();
      switch (h.info().name()) {
         case "ACCBANDS":
            return core.ACCBANDS(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOpt(0), h.realOutput(0), h.realOutput(1), h.realOutput(2));
         case "ACOS":
            return core.ACOS(
               startIdx, endIdx, h.realInput(0), h.realOutput(0));
         case "AD":
            return core.AD(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.price(0, 4), h.realOutput(0));
         case "ADD":
            return core.ADD(
               startIdx, endIdx, h.realInput(0), h.realInput(1), h.realOutput(0));
         case "ADOSC":
            return core.ADOSC(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.price(0, 4), h.intOpt(0), h.intOpt(1), h.realOutput(0));
         case "ADX":
            return core.ADX(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOpt(0), h.realOutput(0));
         case "ADXR":
            return core.ADXR(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOpt(0), h.realOutput(0));
         case "APO":
            return core.APO(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.intOpt(1), h.maTypeOpt(2), h.realOutput(0));
         case "AROON":
            return core.AROON(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.intOpt(0), h.realOutput(0), h.realOutput(1));
         case "AROONOSC":
            return core.AROONOSC(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.intOpt(0), h.realOutput(0));
         case "ASIN":
            return core.ASIN(
               startIdx, endIdx, h.realInput(0), h.realOutput(0));
         case "ATAN":
            return core.ATAN(
               startIdx, endIdx, h.realInput(0), h.realOutput(0));
         case "ATR":
            return core.ATR(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOpt(0), h.realOutput(0));
         case "AVGDEV":
            return core.AVGDEV(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case "AVGPRICE":
            return core.AVGPRICE(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.realOutput(0));
         case "BBANDS":
            return core.BBANDS(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOpt(1), h.realOpt(2), h.maTypeOpt(3), h.realOutput(0), h.realOutput(1), h.realOutput(2));
         case "BETA":
            return core.BETA(
               startIdx, endIdx, h.realInput(0), h.realInput(1), h.intOpt(0), h.realOutput(0));
         case "BOP":
            return core.BOP(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.realOutput(0));
         case "CCI":
            return core.CCI(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOpt(0), h.realOutput(0));
         case "CDL2CROWS":
            return core.CDL2CROWS(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDL3BLACKCROWS":
            return core.CDL3BLACKCROWS(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDL3INSIDE":
            return core.CDL3INSIDE(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDL3LINESTRIKE":
            return core.CDL3LINESTRIKE(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDL3OUTSIDE":
            return core.CDL3OUTSIDE(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDL3STARSINSOUTH":
            return core.CDL3STARSINSOUTH(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDL3WHITESOLDIERS":
            return core.CDL3WHITESOLDIERS(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLABANDONEDBABY":
            return core.CDLABANDONEDBABY(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.realOpt(0), h.intOutput(0));
         case "CDLADVANCEBLOCK":
            return core.CDLADVANCEBLOCK(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLBELTHOLD":
            return core.CDLBELTHOLD(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLBREAKAWAY":
            return core.CDLBREAKAWAY(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLCLOSINGMARUBOZU":
            return core.CDLCLOSINGMARUBOZU(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLCONCEALBABYSWALL":
            return core.CDLCONCEALBABYSWALL(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLCOUNTERATTACK":
            return core.CDLCOUNTERATTACK(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLDARKCLOUDCOVER":
            return core.CDLDARKCLOUDCOVER(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.realOpt(0), h.intOutput(0));
         case "CDLDOJI":
            return core.CDLDOJI(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLDOJISTAR":
            return core.CDLDOJISTAR(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLDRAGONFLYDOJI":
            return core.CDLDRAGONFLYDOJI(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLENGULFING":
            return core.CDLENGULFING(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLEVENINGDOJISTAR":
            return core.CDLEVENINGDOJISTAR(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.realOpt(0), h.intOutput(0));
         case "CDLEVENINGSTAR":
            return core.CDLEVENINGSTAR(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.realOpt(0), h.intOutput(0));
         case "CDLGAPSIDESIDEWHITE":
            return core.CDLGAPSIDESIDEWHITE(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLGRAVESTONEDOJI":
            return core.CDLGRAVESTONEDOJI(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLHAMMER":
            return core.CDLHAMMER(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLHANGINGMAN":
            return core.CDLHANGINGMAN(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLHARAMI":
            return core.CDLHARAMI(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLHARAMICROSS":
            return core.CDLHARAMICROSS(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLHIGHWAVE":
            return core.CDLHIGHWAVE(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLHIKKAKE":
            return core.CDLHIKKAKE(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLHIKKAKEMOD":
            return core.CDLHIKKAKEMOD(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLHOMINGPIGEON":
            return core.CDLHOMINGPIGEON(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLIDENTICAL3CROWS":
            return core.CDLIDENTICAL3CROWS(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLINNECK":
            return core.CDLINNECK(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLINVERTEDHAMMER":
            return core.CDLINVERTEDHAMMER(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLKICKING":
            return core.CDLKICKING(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLKICKINGBYLENGTH":
            return core.CDLKICKINGBYLENGTH(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLLADDERBOTTOM":
            return core.CDLLADDERBOTTOM(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLLONGLEGGEDDOJI":
            return core.CDLLONGLEGGEDDOJI(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLLONGLINE":
            return core.CDLLONGLINE(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLMARUBOZU":
            return core.CDLMARUBOZU(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLMATCHINGLOW":
            return core.CDLMATCHINGLOW(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLMATHOLD":
            return core.CDLMATHOLD(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.realOpt(0), h.intOutput(0));
         case "CDLMORNINGDOJISTAR":
            return core.CDLMORNINGDOJISTAR(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.realOpt(0), h.intOutput(0));
         case "CDLMORNINGSTAR":
            return core.CDLMORNINGSTAR(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.realOpt(0), h.intOutput(0));
         case "CDLONNECK":
            return core.CDLONNECK(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLPIERCING":
            return core.CDLPIERCING(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLRICKSHAWMAN":
            return core.CDLRICKSHAWMAN(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLRISEFALL3METHODS":
            return core.CDLRISEFALL3METHODS(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLSEPARATINGLINES":
            return core.CDLSEPARATINGLINES(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLSHOOTINGSTAR":
            return core.CDLSHOOTINGSTAR(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLSHORTLINE":
            return core.CDLSHORTLINE(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLSPINNINGTOP":
            return core.CDLSPINNINGTOP(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLSTALLEDPATTERN":
            return core.CDLSTALLEDPATTERN(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLSTICKSANDWICH":
            return core.CDLSTICKSANDWICH(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLTAKURI":
            return core.CDLTAKURI(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLTASUKIGAP":
            return core.CDLTASUKIGAP(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLTHRUSTING":
            return core.CDLTHRUSTING(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLTRISTAR":
            return core.CDLTRISTAR(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLUNIQUE3RIVER":
            return core.CDLUNIQUE3RIVER(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLUPSIDEGAP2CROWS":
            return core.CDLUPSIDEGAP2CROWS(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLXSIDEGAP3METHODS":
            return core.CDLXSIDEGAP3METHODS(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CEIL":
            return core.CEIL(
               startIdx, endIdx, h.realInput(0), h.realOutput(0));
         case "CMF":
            return core.CMF(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.price(0, 4), h.intOpt(0), h.realOutput(0));
         case "CMO":
            return core.CMO(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case "CMOU":
            return core.CMOU(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case "CORREL":
            return core.CORREL(
               startIdx, endIdx, h.realInput(0), h.realInput(1), h.intOpt(0), h.realOutput(0));
         case "COS":
            return core.COS(
               startIdx, endIdx, h.realInput(0), h.realOutput(0));
         case "COSH":
            return core.COSH(
               startIdx, endIdx, h.realInput(0), h.realOutput(0));
         case "DEMA":
            return core.DEMA(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case "DIV":
            return core.DIV(
               startIdx, endIdx, h.realInput(0), h.realInput(1), h.realOutput(0));
         case "DX":
            return core.DX(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOpt(0), h.realOutput(0));
         case "EFI":
            return core.EFI(
               startIdx, endIdx, h.price(0, 3), h.price(0, 4), h.intOpt(0), h.realOutput(0));
         case "EMA":
            return core.EMA(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case "EXP":
            return core.EXP(
               startIdx, endIdx, h.realInput(0), h.realOutput(0));
         case "FLOOR":
            return core.FLOOR(
               startIdx, endIdx, h.realInput(0), h.realOutput(0));
         case "HMA":
            return core.HMA(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case "HT_DCPERIOD":
            return core.HT_DCPERIOD(
               startIdx, endIdx, h.realInput(0), h.realOutput(0));
         case "HT_DCPHASE":
            return core.HT_DCPHASE(
               startIdx, endIdx, h.realInput(0), h.realOutput(0));
         case "HT_PHASOR":
            return core.HT_PHASOR(
               startIdx, endIdx, h.realInput(0), h.realOutput(0), h.realOutput(1));
         case "HT_SINE":
            return core.HT_SINE(
               startIdx, endIdx, h.realInput(0), h.realOutput(0), h.realOutput(1));
         case "HT_TRENDLINE":
            return core.HT_TRENDLINE(
               startIdx, endIdx, h.realInput(0), h.realOutput(0));
         case "HT_TRENDMODE":
            return core.HT_TRENDMODE(
               startIdx, endIdx, h.realInput(0), h.intOutput(0));
         case "IMI":
            return core.IMI(
               startIdx, endIdx, h.price(0, 0), h.price(0, 3), h.intOpt(0), h.realOutput(0));
         case "KAMA":
            return core.KAMA(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case "LINEARREG":
            return core.LINEARREG(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case "LINEARREG_ANGLE":
            return core.LINEARREG_ANGLE(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case "LINEARREG_INTERCEPT":
            return core.LINEARREG_INTERCEPT(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case "LINEARREG_SLOPE":
            return core.LINEARREG_SLOPE(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case "LN":
            return core.LN(
               startIdx, endIdx, h.realInput(0), h.realOutput(0));
         case "LOG10":
            return core.LOG10(
               startIdx, endIdx, h.realInput(0), h.realOutput(0));
         case "MA":
            return core.MA(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.maTypeOpt(1), h.realOutput(0));
         case "MACD":
            return core.MACD(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.intOpt(1), h.intOpt(2), h.realOutput(0), h.realOutput(1), h.realOutput(2));
         case "MACDEXT":
            return core.MACDEXT(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.maTypeOpt(1), h.intOpt(2), h.maTypeOpt(3), h.intOpt(4), h.maTypeOpt(5), h.realOutput(0), h.realOutput(1), h.realOutput(2));
         case "MACDFIX":
            return core.MACDFIX(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0), h.realOutput(1), h.realOutput(2));
         case "MAMA":
            return core.MAMA(
               startIdx, endIdx, h.realInput(0), h.realOpt(0), h.realOpt(1), h.realOutput(0), h.realOutput(1));
         case "MARKETFI":
            return core.MARKETFI(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 4), h.realOutput(0));
         case "MAVP":
            return core.MAVP(
               startIdx, endIdx, h.realInput(0), h.realInput(1), h.intOpt(0), h.intOpt(1), h.maTypeOpt(2), h.realOutput(0));
         case "MAX":
            return core.MAX(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case "MAXINDEX":
            return core.MAXINDEX(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.intOutput(0));
         case "MEDPRICE":
            return core.MEDPRICE(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.realOutput(0));
         case "MFI":
            return core.MFI(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.price(0, 4), h.intOpt(0), h.realOutput(0));
         case "MIDPOINT":
            return core.MIDPOINT(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case "MIDPRICE":
            return core.MIDPRICE(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.intOpt(0), h.realOutput(0));
         case "MIN":
            return core.MIN(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case "MININDEX":
            return core.MININDEX(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.intOutput(0));
         case "MINMAX":
            return core.MINMAX(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0), h.realOutput(1));
         case "MINMAXINDEX":
            return core.MINMAXINDEX(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.intOutput(0), h.intOutput(1));
         case "MINUS_DI":
            return core.MINUS_DI(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOpt(0), h.realOutput(0));
         case "MINUS_DM":
            return core.MINUS_DM(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.intOpt(0), h.realOutput(0));
         case "MOM":
            return core.MOM(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case "MULT":
            return core.MULT(
               startIdx, endIdx, h.realInput(0), h.realInput(1), h.realOutput(0));
         case "NATR":
            return core.NATR(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOpt(0), h.realOutput(0));
         case "NVI":
            return core.NVI(
               startIdx, endIdx, h.price(0, 3), h.price(0, 4), h.realOutput(0));
         case "OBV":
            return core.OBV(
               startIdx, endIdx, h.realInput(0), h.price(1, 4), h.realOutput(0));
         case "PLUS_DI":
            return core.PLUS_DI(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOpt(0), h.realOutput(0));
         case "PLUS_DM":
            return core.PLUS_DM(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.intOpt(0), h.realOutput(0));
         case "PPO":
            return core.PPO(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.intOpt(1), h.maTypeOpt(2), h.realOutput(0));
         case "PVI":
            return core.PVI(
               startIdx, endIdx, h.price(0, 3), h.price(0, 4), h.realOutput(0));
         case "PVO":
            return core.PVO(
               startIdx, endIdx, h.price(0, 4), h.intOpt(0), h.intOpt(1), h.maTypeOpt(2), h.realOutput(0));
         case "QSTICK":
            return core.QSTICK(
               startIdx, endIdx, h.price(0, 0), h.price(0, 3), h.intOpt(0), h.realOutput(0));
         case "ROC":
            return core.ROC(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case "ROCP":
            return core.ROCP(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case "ROCR":
            return core.ROCR(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case "ROCR100":
            return core.ROCR100(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case "RSI":
            return core.RSI(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case "SAR":
            return core.SAR(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.realOpt(0), h.realOpt(1), h.realOutput(0));
         case "SAREXT":
            return core.SAREXT(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.realOpt(0), h.realOpt(1), h.realOpt(2), h.realOpt(3), h.realOpt(4), h.realOpt(5), h.realOpt(6), h.realOpt(7), h.realOutput(0));
         case "SIN":
            return core.SIN(
               startIdx, endIdx, h.realInput(0), h.realOutput(0));
         case "SINH":
            return core.SINH(
               startIdx, endIdx, h.realInput(0), h.realOutput(0));
         case "SMA":
            return core.SMA(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case "SQRT":
            return core.SQRT(
               startIdx, endIdx, h.realInput(0), h.realOutput(0));
         case "STDDEV":
            return core.STDDEV(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOpt(1), h.realOutput(0));
         case "STOCH":
            return core.STOCH(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOpt(0), h.intOpt(1), h.maTypeOpt(2), h.intOpt(3), h.maTypeOpt(4), h.realOutput(0), h.realOutput(1));
         case "STOCHF":
            return core.STOCHF(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOpt(0), h.intOpt(1), h.maTypeOpt(2), h.realOutput(0), h.realOutput(1));
         case "STOCHRSI":
            return core.STOCHRSI(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.intOpt(1), h.intOpt(2), h.maTypeOpt(3), h.realOutput(0), h.realOutput(1));
         case "SUB":
            return core.SUB(
               startIdx, endIdx, h.realInput(0), h.realInput(1), h.realOutput(0));
         case "SUM":
            return core.SUM(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case "T3":
            return core.T3(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOpt(1), h.realOutput(0));
         case "TAN":
            return core.TAN(
               startIdx, endIdx, h.realInput(0), h.realOutput(0));
         case "TANH":
            return core.TANH(
               startIdx, endIdx, h.realInput(0), h.realOutput(0));
         case "TEMA":
            return core.TEMA(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case "TRANGE":
            return core.TRANGE(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.realOutput(0));
         case "TRIMA":
            return core.TRIMA(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case "TRIX":
            return core.TRIX(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case "TSF":
            return core.TSF(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case "TYPPRICE":
            return core.TYPPRICE(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.realOutput(0));
         case "ULTOSC":
            return core.ULTOSC(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOpt(0), h.intOpt(1), h.intOpt(2), h.realOutput(0));
         case "VAR":
            return core.VAR(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOpt(1), h.realOutput(0));
         case "VWMA":
            return core.VWMA(
               startIdx, endIdx, h.realInput(0), h.price(1, 4), h.intOpt(0), h.realOutput(0));
         case "WAD":
            return core.WAD(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.realOutput(0));
         case "WCLPRICE":
            return core.WCLPRICE(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.realOutput(0));
         case "WILLR":
            return core.WILLR(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOpt(0), h.realOutput(0));
         case "WMA":
            return core.WMA(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         default:
            throw new IllegalArgumentException("no such function: " + h.info().name());
      }
   }

   /* The lookback tier. Separate from call() because it takes only the optional
      parameters -- no inputs, no outputs, no range -- so a caller can size its
      output arrays before binding them, exactly as TA_GetLookback allows. */
   static int lookback(ParamHolder h) {
      Core core = h.core();
      switch (h.info().name()) {
         case "ACCBANDS":
            return core.ACCBANDS_Lookback(h.intOpt(0));
         case "ACOS":
            return core.ACOS_Lookback();
         case "AD":
            return core.AD_Lookback();
         case "ADD":
            return core.ADD_Lookback();
         case "ADOSC":
            return core.ADOSC_Lookback(h.intOpt(0), h.intOpt(1));
         case "ADX":
            return core.ADX_Lookback(h.intOpt(0));
         case "ADXR":
            return core.ADXR_Lookback(h.intOpt(0));
         case "APO":
            return core.APO_Lookback(h.intOpt(0), h.intOpt(1), h.maTypeOpt(2));
         case "AROON":
            return core.AROON_Lookback(h.intOpt(0));
         case "AROONOSC":
            return core.AROONOSC_Lookback(h.intOpt(0));
         case "ASIN":
            return core.ASIN_Lookback();
         case "ATAN":
            return core.ATAN_Lookback();
         case "ATR":
            return core.ATR_Lookback(h.intOpt(0));
         case "AVGDEV":
            return core.AVGDEV_Lookback(h.intOpt(0));
         case "AVGPRICE":
            return core.AVGPRICE_Lookback();
         case "BBANDS":
            return core.BBANDS_Lookback(h.intOpt(0), h.realOpt(1), h.realOpt(2), h.maTypeOpt(3));
         case "BETA":
            return core.BETA_Lookback(h.intOpt(0));
         case "BOP":
            return core.BOP_Lookback();
         case "CCI":
            return core.CCI_Lookback(h.intOpt(0));
         case "CDL2CROWS":
            return core.CDL2CROWS_Lookback();
         case "CDL3BLACKCROWS":
            return core.CDL3BLACKCROWS_Lookback();
         case "CDL3INSIDE":
            return core.CDL3INSIDE_Lookback();
         case "CDL3LINESTRIKE":
            return core.CDL3LINESTRIKE_Lookback();
         case "CDL3OUTSIDE":
            return core.CDL3OUTSIDE_Lookback();
         case "CDL3STARSINSOUTH":
            return core.CDL3STARSINSOUTH_Lookback();
         case "CDL3WHITESOLDIERS":
            return core.CDL3WHITESOLDIERS_Lookback();
         case "CDLABANDONEDBABY":
            return core.CDLABANDONEDBABY_Lookback(h.realOpt(0));
         case "CDLADVANCEBLOCK":
            return core.CDLADVANCEBLOCK_Lookback();
         case "CDLBELTHOLD":
            return core.CDLBELTHOLD_Lookback();
         case "CDLBREAKAWAY":
            return core.CDLBREAKAWAY_Lookback();
         case "CDLCLOSINGMARUBOZU":
            return core.CDLCLOSINGMARUBOZU_Lookback();
         case "CDLCONCEALBABYSWALL":
            return core.CDLCONCEALBABYSWALL_Lookback();
         case "CDLCOUNTERATTACK":
            return core.CDLCOUNTERATTACK_Lookback();
         case "CDLDARKCLOUDCOVER":
            return core.CDLDARKCLOUDCOVER_Lookback(h.realOpt(0));
         case "CDLDOJI":
            return core.CDLDOJI_Lookback();
         case "CDLDOJISTAR":
            return core.CDLDOJISTAR_Lookback();
         case "CDLDRAGONFLYDOJI":
            return core.CDLDRAGONFLYDOJI_Lookback();
         case "CDLENGULFING":
            return core.CDLENGULFING_Lookback();
         case "CDLEVENINGDOJISTAR":
            return core.CDLEVENINGDOJISTAR_Lookback(h.realOpt(0));
         case "CDLEVENINGSTAR":
            return core.CDLEVENINGSTAR_Lookback(h.realOpt(0));
         case "CDLGAPSIDESIDEWHITE":
            return core.CDLGAPSIDESIDEWHITE_Lookback();
         case "CDLGRAVESTONEDOJI":
            return core.CDLGRAVESTONEDOJI_Lookback();
         case "CDLHAMMER":
            return core.CDLHAMMER_Lookback();
         case "CDLHANGINGMAN":
            return core.CDLHANGINGMAN_Lookback();
         case "CDLHARAMI":
            return core.CDLHARAMI_Lookback();
         case "CDLHARAMICROSS":
            return core.CDLHARAMICROSS_Lookback();
         case "CDLHIGHWAVE":
            return core.CDLHIGHWAVE_Lookback();
         case "CDLHIKKAKE":
            return core.CDLHIKKAKE_Lookback();
         case "CDLHIKKAKEMOD":
            return core.CDLHIKKAKEMOD_Lookback();
         case "CDLHOMINGPIGEON":
            return core.CDLHOMINGPIGEON_Lookback();
         case "CDLIDENTICAL3CROWS":
            return core.CDLIDENTICAL3CROWS_Lookback();
         case "CDLINNECK":
            return core.CDLINNECK_Lookback();
         case "CDLINVERTEDHAMMER":
            return core.CDLINVERTEDHAMMER_Lookback();
         case "CDLKICKING":
            return core.CDLKICKING_Lookback();
         case "CDLKICKINGBYLENGTH":
            return core.CDLKICKINGBYLENGTH_Lookback();
         case "CDLLADDERBOTTOM":
            return core.CDLLADDERBOTTOM_Lookback();
         case "CDLLONGLEGGEDDOJI":
            return core.CDLLONGLEGGEDDOJI_Lookback();
         case "CDLLONGLINE":
            return core.CDLLONGLINE_Lookback();
         case "CDLMARUBOZU":
            return core.CDLMARUBOZU_Lookback();
         case "CDLMATCHINGLOW":
            return core.CDLMATCHINGLOW_Lookback();
         case "CDLMATHOLD":
            return core.CDLMATHOLD_Lookback(h.realOpt(0));
         case "CDLMORNINGDOJISTAR":
            return core.CDLMORNINGDOJISTAR_Lookback(h.realOpt(0));
         case "CDLMORNINGSTAR":
            return core.CDLMORNINGSTAR_Lookback(h.realOpt(0));
         case "CDLONNECK":
            return core.CDLONNECK_Lookback();
         case "CDLPIERCING":
            return core.CDLPIERCING_Lookback();
         case "CDLRICKSHAWMAN":
            return core.CDLRICKSHAWMAN_Lookback();
         case "CDLRISEFALL3METHODS":
            return core.CDLRISEFALL3METHODS_Lookback();
         case "CDLSEPARATINGLINES":
            return core.CDLSEPARATINGLINES_Lookback();
         case "CDLSHOOTINGSTAR":
            return core.CDLSHOOTINGSTAR_Lookback();
         case "CDLSHORTLINE":
            return core.CDLSHORTLINE_Lookback();
         case "CDLSPINNINGTOP":
            return core.CDLSPINNINGTOP_Lookback();
         case "CDLSTALLEDPATTERN":
            return core.CDLSTALLEDPATTERN_Lookback();
         case "CDLSTICKSANDWICH":
            return core.CDLSTICKSANDWICH_Lookback();
         case "CDLTAKURI":
            return core.CDLTAKURI_Lookback();
         case "CDLTASUKIGAP":
            return core.CDLTASUKIGAP_Lookback();
         case "CDLTHRUSTING":
            return core.CDLTHRUSTING_Lookback();
         case "CDLTRISTAR":
            return core.CDLTRISTAR_Lookback();
         case "CDLUNIQUE3RIVER":
            return core.CDLUNIQUE3RIVER_Lookback();
         case "CDLUPSIDEGAP2CROWS":
            return core.CDLUPSIDEGAP2CROWS_Lookback();
         case "CDLXSIDEGAP3METHODS":
            return core.CDLXSIDEGAP3METHODS_Lookback();
         case "CEIL":
            return core.CEIL_Lookback();
         case "CMF":
            return core.CMF_Lookback(h.intOpt(0));
         case "CMO":
            return core.CMO_Lookback(h.intOpt(0));
         case "CMOU":
            return core.CMOU_Lookback(h.intOpt(0));
         case "CORREL":
            return core.CORREL_Lookback(h.intOpt(0));
         case "COS":
            return core.COS_Lookback();
         case "COSH":
            return core.COSH_Lookback();
         case "DEMA":
            return core.DEMA_Lookback(h.intOpt(0));
         case "DIV":
            return core.DIV_Lookback();
         case "DX":
            return core.DX_Lookback(h.intOpt(0));
         case "EFI":
            return core.EFI_Lookback(h.intOpt(0));
         case "EMA":
            return core.EMA_Lookback(h.intOpt(0));
         case "EXP":
            return core.EXP_Lookback();
         case "FLOOR":
            return core.FLOOR_Lookback();
         case "HMA":
            return core.HMA_Lookback(h.intOpt(0));
         case "HT_DCPERIOD":
            return core.HT_DCPERIOD_Lookback();
         case "HT_DCPHASE":
            return core.HT_DCPHASE_Lookback();
         case "HT_PHASOR":
            return core.HT_PHASOR_Lookback();
         case "HT_SINE":
            return core.HT_SINE_Lookback();
         case "HT_TRENDLINE":
            return core.HT_TRENDLINE_Lookback();
         case "HT_TRENDMODE":
            return core.HT_TRENDMODE_Lookback();
         case "IMI":
            return core.IMI_Lookback(h.intOpt(0));
         case "KAMA":
            return core.KAMA_Lookback(h.intOpt(0));
         case "LINEARREG":
            return core.LINEARREG_Lookback(h.intOpt(0));
         case "LINEARREG_ANGLE":
            return core.LINEARREG_ANGLE_Lookback(h.intOpt(0));
         case "LINEARREG_INTERCEPT":
            return core.LINEARREG_INTERCEPT_Lookback(h.intOpt(0));
         case "LINEARREG_SLOPE":
            return core.LINEARREG_SLOPE_Lookback(h.intOpt(0));
         case "LN":
            return core.LN_Lookback();
         case "LOG10":
            return core.LOG10_Lookback();
         case "MA":
            return core.MA_Lookback(h.intOpt(0), h.maTypeOpt(1));
         case "MACD":
            return core.MACD_Lookback(h.intOpt(0), h.intOpt(1), h.intOpt(2));
         case "MACDEXT":
            return core.MACDEXT_Lookback(h.intOpt(0), h.maTypeOpt(1), h.intOpt(2), h.maTypeOpt(3), h.intOpt(4), h.maTypeOpt(5));
         case "MACDFIX":
            return core.MACDFIX_Lookback(h.intOpt(0));
         case "MAMA":
            return core.MAMA_Lookback(h.realOpt(0), h.realOpt(1));
         case "MARKETFI":
            return core.MARKETFI_Lookback();
         case "MAVP":
            return core.MAVP_Lookback(h.intOpt(0), h.intOpt(1), h.maTypeOpt(2));
         case "MAX":
            return core.MAX_Lookback(h.intOpt(0));
         case "MAXINDEX":
            return core.MAXINDEX_Lookback(h.intOpt(0));
         case "MEDPRICE":
            return core.MEDPRICE_Lookback();
         case "MFI":
            return core.MFI_Lookback(h.intOpt(0));
         case "MIDPOINT":
            return core.MIDPOINT_Lookback(h.intOpt(0));
         case "MIDPRICE":
            return core.MIDPRICE_Lookback(h.intOpt(0));
         case "MIN":
            return core.MIN_Lookback(h.intOpt(0));
         case "MININDEX":
            return core.MININDEX_Lookback(h.intOpt(0));
         case "MINMAX":
            return core.MINMAX_Lookback(h.intOpt(0));
         case "MINMAXINDEX":
            return core.MINMAXINDEX_Lookback(h.intOpt(0));
         case "MINUS_DI":
            return core.MINUS_DI_Lookback(h.intOpt(0));
         case "MINUS_DM":
            return core.MINUS_DM_Lookback(h.intOpt(0));
         case "MOM":
            return core.MOM_Lookback(h.intOpt(0));
         case "MULT":
            return core.MULT_Lookback();
         case "NATR":
            return core.NATR_Lookback(h.intOpt(0));
         case "NVI":
            return core.NVI_Lookback();
         case "OBV":
            return core.OBV_Lookback();
         case "PLUS_DI":
            return core.PLUS_DI_Lookback(h.intOpt(0));
         case "PLUS_DM":
            return core.PLUS_DM_Lookback(h.intOpt(0));
         case "PPO":
            return core.PPO_Lookback(h.intOpt(0), h.intOpt(1), h.maTypeOpt(2));
         case "PVI":
            return core.PVI_Lookback();
         case "PVO":
            return core.PVO_Lookback(h.intOpt(0), h.intOpt(1), h.maTypeOpt(2));
         case "QSTICK":
            return core.QSTICK_Lookback(h.intOpt(0));
         case "ROC":
            return core.ROC_Lookback(h.intOpt(0));
         case "ROCP":
            return core.ROCP_Lookback(h.intOpt(0));
         case "ROCR":
            return core.ROCR_Lookback(h.intOpt(0));
         case "ROCR100":
            return core.ROCR100_Lookback(h.intOpt(0));
         case "RSI":
            return core.RSI_Lookback(h.intOpt(0));
         case "SAR":
            return core.SAR_Lookback(h.realOpt(0), h.realOpt(1));
         case "SAREXT":
            return core.SAREXT_Lookback(h.realOpt(0), h.realOpt(1), h.realOpt(2), h.realOpt(3), h.realOpt(4), h.realOpt(5), h.realOpt(6), h.realOpt(7));
         case "SIN":
            return core.SIN_Lookback();
         case "SINH":
            return core.SINH_Lookback();
         case "SMA":
            return core.SMA_Lookback(h.intOpt(0));
         case "SQRT":
            return core.SQRT_Lookback();
         case "STDDEV":
            return core.STDDEV_Lookback(h.intOpt(0), h.realOpt(1));
         case "STOCH":
            return core.STOCH_Lookback(h.intOpt(0), h.intOpt(1), h.maTypeOpt(2), h.intOpt(3), h.maTypeOpt(4));
         case "STOCHF":
            return core.STOCHF_Lookback(h.intOpt(0), h.intOpt(1), h.maTypeOpt(2));
         case "STOCHRSI":
            return core.STOCHRSI_Lookback(h.intOpt(0), h.intOpt(1), h.intOpt(2), h.maTypeOpt(3));
         case "SUB":
            return core.SUB_Lookback();
         case "SUM":
            return core.SUM_Lookback(h.intOpt(0));
         case "T3":
            return core.T3_Lookback(h.intOpt(0), h.realOpt(1));
         case "TAN":
            return core.TAN_Lookback();
         case "TANH":
            return core.TANH_Lookback();
         case "TEMA":
            return core.TEMA_Lookback(h.intOpt(0));
         case "TRANGE":
            return core.TRANGE_Lookback();
         case "TRIMA":
            return core.TRIMA_Lookback(h.intOpt(0));
         case "TRIX":
            return core.TRIX_Lookback(h.intOpt(0));
         case "TSF":
            return core.TSF_Lookback(h.intOpt(0));
         case "TYPPRICE":
            return core.TYPPRICE_Lookback();
         case "ULTOSC":
            return core.ULTOSC_Lookback(h.intOpt(0), h.intOpt(1), h.intOpt(2));
         case "VAR":
            return core.VAR_Lookback(h.intOpt(0), h.realOpt(1));
         case "VWMA":
            return core.VWMA_Lookback(h.intOpt(0));
         case "WAD":
            return core.WAD_Lookback();
         case "WCLPRICE":
            return core.WCLPRICE_Lookback();
         case "WILLR":
            return core.WILLR_Lookback(h.intOpt(0));
         case "WMA":
            return core.WMA_Lookback(h.intOpt(0));
         default:
            throw new IllegalArgumentException("no such function: " + h.info().name());
      }
   }
}
