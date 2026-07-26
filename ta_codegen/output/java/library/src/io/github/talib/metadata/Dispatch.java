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
            return core.accbands(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOpt(0), h.realOutput(0), h.realOutput(1), h.realOutput(2));
         case "ACOS":
            return core.acos(
               startIdx, endIdx, h.realInput(0), h.realOutput(0));
         case "AD":
            return core.ad(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.price(0, 4), h.realOutput(0));
         case "ADD":
            return core.add(
               startIdx, endIdx, h.realInput(0), h.realInput(1), h.realOutput(0));
         case "ADOSC":
            return core.adOsc(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.price(0, 4), h.intOpt(0), h.intOpt(1), h.realOutput(0));
         case "ADX":
            return core.adx(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOpt(0), h.realOutput(0));
         case "ADXR":
            return core.adxr(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOpt(0), h.realOutput(0));
         case "APO":
            return core.apo(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.intOpt(1), h.maTypeOpt(2), h.realOutput(0));
         case "AROON":
            return core.aroon(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.intOpt(0), h.realOutput(0), h.realOutput(1));
         case "AROONOSC":
            return core.aroonOsc(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.intOpt(0), h.realOutput(0));
         case "ASIN":
            return core.asin(
               startIdx, endIdx, h.realInput(0), h.realOutput(0));
         case "ATAN":
            return core.atan(
               startIdx, endIdx, h.realInput(0), h.realOutput(0));
         case "ATR":
            return core.atr(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOpt(0), h.realOutput(0));
         case "AVGDEV":
            return core.avgDev(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case "AVGPRICE":
            return core.avgPrice(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.realOutput(0));
         case "BBANDS":
            return core.bbands(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOpt(1), h.realOpt(2), h.maTypeOpt(3), h.realOutput(0), h.realOutput(1), h.realOutput(2));
         case "BETA":
            return core.beta(
               startIdx, endIdx, h.realInput(0), h.realInput(1), h.intOpt(0), h.realOutput(0));
         case "BOP":
            return core.bop(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.realOutput(0));
         case "CCI":
            return core.cci(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOpt(0), h.realOutput(0));
         case "CDL2CROWS":
            return core.cdl2Crows(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDL3BLACKCROWS":
            return core.cdl3BlackCrows(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDL3INSIDE":
            return core.cdl3Inside(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDL3LINESTRIKE":
            return core.cdl3LineStrike(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDL3OUTSIDE":
            return core.cdl3Outside(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDL3STARSINSOUTH":
            return core.cdl3StarsInSouth(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDL3WHITESOLDIERS":
            return core.cdl3WhiteSoldiers(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLABANDONEDBABY":
            return core.cdlAbandonedBaby(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.realOpt(0), h.intOutput(0));
         case "CDLADVANCEBLOCK":
            return core.cdlAdvanceBlock(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLBELTHOLD":
            return core.cdlBeltHold(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLBREAKAWAY":
            return core.cdlBreakaway(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLCLOSINGMARUBOZU":
            return core.cdlClosingMarubozu(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLCONCEALBABYSWALL":
            return core.cdlConcealBabysWall(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLCOUNTERATTACK":
            return core.cdlCounterAttack(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLDARKCLOUDCOVER":
            return core.cdlDarkCloudCover(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.realOpt(0), h.intOutput(0));
         case "CDLDOJI":
            return core.cdlDoji(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLDOJISTAR":
            return core.cdlDojiStar(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLDRAGONFLYDOJI":
            return core.cdlDragonflyDoji(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLENGULFING":
            return core.cdlEngulfing(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLEVENINGDOJISTAR":
            return core.cdlEveningDojiStar(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.realOpt(0), h.intOutput(0));
         case "CDLEVENINGSTAR":
            return core.cdlEveningStar(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.realOpt(0), h.intOutput(0));
         case "CDLGAPSIDESIDEWHITE":
            return core.cdlGapSideSideWhite(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLGRAVESTONEDOJI":
            return core.cdlGravestoneDoji(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLHAMMER":
            return core.cdlHammer(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLHANGINGMAN":
            return core.cdlHangingMan(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLHARAMI":
            return core.cdlHarami(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLHARAMICROSS":
            return core.cdlHaramiCross(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLHIGHWAVE":
            return core.cdlHignWave(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLHIKKAKE":
            return core.cdlHikkake(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLHIKKAKEMOD":
            return core.cdlHikkakeMod(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLHOMINGPIGEON":
            return core.cdlHomingPigeon(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLIDENTICAL3CROWS":
            return core.cdlIdentical3Crows(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLINNECK":
            return core.cdlInNeck(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLINVERTEDHAMMER":
            return core.cdlInvertedHammer(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLKICKING":
            return core.cdlKicking(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLKICKINGBYLENGTH":
            return core.cdlKickingByLength(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLLADDERBOTTOM":
            return core.cdlLadderBottom(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLLONGLEGGEDDOJI":
            return core.cdlLongLeggedDoji(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLLONGLINE":
            return core.cdlLongLine(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLMARUBOZU":
            return core.cdlMarubozu(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLMATCHINGLOW":
            return core.cdlMatchingLow(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLMATHOLD":
            return core.cdlMatHold(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.realOpt(0), h.intOutput(0));
         case "CDLMORNINGDOJISTAR":
            return core.cdlMorningDojiStar(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.realOpt(0), h.intOutput(0));
         case "CDLMORNINGSTAR":
            return core.cdlMorningStar(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.realOpt(0), h.intOutput(0));
         case "CDLONNECK":
            return core.cdlOnNeck(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLPIERCING":
            return core.cdlPiercing(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLRICKSHAWMAN":
            return core.cdlRickshawMan(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLRISEFALL3METHODS":
            return core.cdlRiseFall3Methods(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLSEPARATINGLINES":
            return core.cdlSeperatingLines(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLSHOOTINGSTAR":
            return core.cdlShootingStar(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLSHORTLINE":
            return core.cdlShortLine(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLSPINNINGTOP":
            return core.cdlSpinningTop(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLSTALLEDPATTERN":
            return core.cdlStalledPattern(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLSTICKSANDWICH":
            return core.cdlStickSandwich(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLTAKURI":
            return core.cdlTakuri(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLTASUKIGAP":
            return core.cdlTasukiGap(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLTHRUSTING":
            return core.cdlThrusting(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLTRISTAR":
            return core.cdlTristar(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLUNIQUE3RIVER":
            return core.cdlUnique3River(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLUPSIDEGAP2CROWS":
            return core.cdlUpsideGap2Crows(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLXSIDEGAP3METHODS":
            return core.cdlXSideGap3Methods(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CEIL":
            return core.ceil(
               startIdx, endIdx, h.realInput(0), h.realOutput(0));
         case "CMF":
            return core.cmf(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.price(0, 4), h.intOpt(0), h.realOutput(0));
         case "CMO":
            return core.cmo(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case "CMOU":
            return core.cmou(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case "CORREL":
            return core.correl(
               startIdx, endIdx, h.realInput(0), h.realInput(1), h.intOpt(0), h.realOutput(0));
         case "COS":
            return core.cos(
               startIdx, endIdx, h.realInput(0), h.realOutput(0));
         case "COSH":
            return core.cosh(
               startIdx, endIdx, h.realInput(0), h.realOutput(0));
         case "DEMA":
            return core.dema(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case "DIV":
            return core.div(
               startIdx, endIdx, h.realInput(0), h.realInput(1), h.realOutput(0));
         case "DX":
            return core.dx(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOpt(0), h.realOutput(0));
         case "EMA":
            return core.ema(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case "EXP":
            return core.exp(
               startIdx, endIdx, h.realInput(0), h.realOutput(0));
         case "FLOOR":
            return core.floor(
               startIdx, endIdx, h.realInput(0), h.realOutput(0));
         case "HMA":
            return core.hma(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case "HT_DCPERIOD":
            return core.htDcPeriod(
               startIdx, endIdx, h.realInput(0), h.realOutput(0));
         case "HT_DCPHASE":
            return core.htDcPhase(
               startIdx, endIdx, h.realInput(0), h.realOutput(0));
         case "HT_PHASOR":
            return core.htPhasor(
               startIdx, endIdx, h.realInput(0), h.realOutput(0), h.realOutput(1));
         case "HT_SINE":
            return core.htSine(
               startIdx, endIdx, h.realInput(0), h.realOutput(0), h.realOutput(1));
         case "HT_TRENDLINE":
            return core.htTrendline(
               startIdx, endIdx, h.realInput(0), h.realOutput(0));
         case "HT_TRENDMODE":
            return core.htTrendMode(
               startIdx, endIdx, h.realInput(0), h.intOutput(0));
         case "IMI":
            return core.imi(
               startIdx, endIdx, h.price(0, 0), h.price(0, 3), h.intOpt(0), h.realOutput(0));
         case "KAMA":
            return core.kama(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case "LINEARREG":
            return core.linearReg(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case "LINEARREG_ANGLE":
            return core.linearRegAngle(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case "LINEARREG_INTERCEPT":
            return core.linearRegIntercept(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case "LINEARREG_SLOPE":
            return core.linearRegSlope(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case "LN":
            return core.ln(
               startIdx, endIdx, h.realInput(0), h.realOutput(0));
         case "LOG10":
            return core.log10(
               startIdx, endIdx, h.realInput(0), h.realOutput(0));
         case "MA":
            return core.movingAverage(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.maTypeOpt(1), h.realOutput(0));
         case "MACD":
            return core.macd(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.intOpt(1), h.intOpt(2), h.realOutput(0), h.realOutput(1), h.realOutput(2));
         case "MACDEXT":
            return core.macdExt(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.maTypeOpt(1), h.intOpt(2), h.maTypeOpt(3), h.intOpt(4), h.maTypeOpt(5), h.realOutput(0), h.realOutput(1), h.realOutput(2));
         case "MACDFIX":
            return core.macdFix(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0), h.realOutput(1), h.realOutput(2));
         case "MAMA":
            return core.mama(
               startIdx, endIdx, h.realInput(0), h.realOpt(0), h.realOpt(1), h.realOutput(0), h.realOutput(1));
         case "MAVP":
            return core.movingAverageVariablePeriod(
               startIdx, endIdx, h.realInput(0), h.realInput(1), h.intOpt(0), h.intOpt(1), h.maTypeOpt(2), h.realOutput(0));
         case "MAX":
            return core.max(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case "MAXINDEX":
            return core.maxIndex(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.intOutput(0));
         case "MEDPRICE":
            return core.medPrice(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.realOutput(0));
         case "MFI":
            return core.mfi(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.price(0, 4), h.intOpt(0), h.realOutput(0));
         case "MIDPOINT":
            return core.midPoint(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case "MIDPRICE":
            return core.midPrice(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.intOpt(0), h.realOutput(0));
         case "MIN":
            return core.min(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case "MININDEX":
            return core.minIndex(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.intOutput(0));
         case "MINMAX":
            return core.minMax(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0), h.realOutput(1));
         case "MINMAXINDEX":
            return core.minMaxIndex(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.intOutput(0), h.intOutput(1));
         case "MINUS_DI":
            return core.minusDI(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOpt(0), h.realOutput(0));
         case "MINUS_DM":
            return core.minusDM(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.intOpt(0), h.realOutput(0));
         case "MOM":
            return core.mom(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case "MULT":
            return core.mult(
               startIdx, endIdx, h.realInput(0), h.realInput(1), h.realOutput(0));
         case "NATR":
            return core.natr(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOpt(0), h.realOutput(0));
         case "NVI":
            return core.nvi(
               startIdx, endIdx, h.price(0, 3), h.price(0, 4), h.realOutput(0));
         case "OBV":
            return core.obv(
               startIdx, endIdx, h.realInput(0), h.price(1, 4), h.realOutput(0));
         case "PLUS_DI":
            return core.plusDI(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOpt(0), h.realOutput(0));
         case "PLUS_DM":
            return core.plusDM(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.intOpt(0), h.realOutput(0));
         case "PPO":
            return core.ppo(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.intOpt(1), h.maTypeOpt(2), h.realOutput(0));
         case "PVI":
            return core.pvi(
               startIdx, endIdx, h.price(0, 3), h.price(0, 4), h.realOutput(0));
         case "PVO":
            return core.pvo(
               startIdx, endIdx, h.price(0, 4), h.intOpt(0), h.intOpt(1), h.maTypeOpt(2), h.realOutput(0));
         case "ROC":
            return core.roc(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case "ROCP":
            return core.rocP(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case "ROCR":
            return core.rocR(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case "ROCR100":
            return core.rocR100(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case "RSI":
            return core.rsi(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case "SAR":
            return core.sar(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.realOpt(0), h.realOpt(1), h.realOutput(0));
         case "SAREXT":
            return core.sarExt(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.realOpt(0), h.realOpt(1), h.realOpt(2), h.realOpt(3), h.realOpt(4), h.realOpt(5), h.realOpt(6), h.realOpt(7), h.realOutput(0));
         case "SIN":
            return core.sin(
               startIdx, endIdx, h.realInput(0), h.realOutput(0));
         case "SINH":
            return core.sinh(
               startIdx, endIdx, h.realInput(0), h.realOutput(0));
         case "SMA":
            return core.sma(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case "SQRT":
            return core.sqrt(
               startIdx, endIdx, h.realInput(0), h.realOutput(0));
         case "STDDEV":
            return core.stdDev(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOpt(1), h.realOutput(0));
         case "STOCH":
            return core.stoch(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOpt(0), h.intOpt(1), h.maTypeOpt(2), h.intOpt(3), h.maTypeOpt(4), h.realOutput(0), h.realOutput(1));
         case "STOCHF":
            return core.stochF(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOpt(0), h.intOpt(1), h.maTypeOpt(2), h.realOutput(0), h.realOutput(1));
         case "STOCHRSI":
            return core.stochRsi(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.intOpt(1), h.intOpt(2), h.maTypeOpt(3), h.realOutput(0), h.realOutput(1));
         case "SUB":
            return core.sub(
               startIdx, endIdx, h.realInput(0), h.realInput(1), h.realOutput(0));
         case "SUM":
            return core.sum(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case "T3":
            return core.t3(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOpt(1), h.realOutput(0));
         case "TAN":
            return core.tan(
               startIdx, endIdx, h.realInput(0), h.realOutput(0));
         case "TANH":
            return core.tanh(
               startIdx, endIdx, h.realInput(0), h.realOutput(0));
         case "TEMA":
            return core.tema(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case "TRANGE":
            return core.trueRange(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.realOutput(0));
         case "TRIMA":
            return core.trima(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case "TRIX":
            return core.trix(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case "TSF":
            return core.tsf(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case "TYPPRICE":
            return core.typPrice(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.realOutput(0));
         case "ULTOSC":
            return core.ultOsc(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOpt(0), h.intOpt(1), h.intOpt(2), h.realOutput(0));
         case "VAR":
            return core.variance(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOpt(1), h.realOutput(0));
         case "VWMA":
            return core.vwma(
               startIdx, endIdx, h.realInput(0), h.price(1, 4), h.intOpt(0), h.realOutput(0));
         case "WCLPRICE":
            return core.wclPrice(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.realOutput(0));
         case "WILLR":
            return core.willR(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOpt(0), h.realOutput(0));
         case "WMA":
            return core.wma(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         default:
            throw new IllegalArgumentException("no such function: " + h.info().name());
      }
   }
}
