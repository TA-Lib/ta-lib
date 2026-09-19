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
         case "AC":
            return core.ac(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.intOpt(0), h.intOpt(1), h.intOpt(2), h.realOutput(0));
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
            return core.adosc(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.price(0, 4), h.intOpt(0), h.intOpt(1), h.realOutput(0));
         case "ADR":
            return core.adr(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.intOpt(0), h.realOutput(0));
         case "ADX":
            return core.adx(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOpt(0), h.realOutput(0));
         case "ADXR":
            return core.adxr(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOpt(0), h.realOutput(0));
         case "AO":
            return core.ao(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.intOpt(0), h.intOpt(1), h.realOutput(0));
         case "APO":
            return core.apo(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.intOpt(1), h.maTypeOpt(2), h.realOutput(0));
         case "AROON":
            return core.aroon(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.intOpt(0), h.realOutput(0), h.realOutput(1));
         case "AROONOSC":
            return core.aroonosc(
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
            return core.avgdev(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case "AVGPRICE":
            return core.avgprice(
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
            return core.cdl2crows(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDL3BLACKCROWS":
            return core.cdl3blackcrows(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDL3INSIDE":
            return core.cdl3inside(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDL3LINESTRIKE":
            return core.cdl3linestrike(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDL3OUTSIDE":
            return core.cdl3outside(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDL3STARSINSOUTH":
            return core.cdl3starsinsouth(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDL3WHITESOLDIERS":
            return core.cdl3whitesoldiers(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLABANDONEDBABY":
            return core.cdlabandonedbaby(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.realOpt(0), h.intOutput(0));
         case "CDLADVANCEBLOCK":
            return core.cdladvanceblock(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLBELTHOLD":
            return core.cdlbelthold(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLBREAKAWAY":
            return core.cdlbreakaway(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLCLOSINGMARUBOZU":
            return core.cdlclosingmarubozu(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLCONCEALBABYSWALL":
            return core.cdlconcealbabyswall(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLCOUNTERATTACK":
            return core.cdlcounterattack(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLDARKCLOUDCOVER":
            return core.cdldarkcloudcover(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.realOpt(0), h.intOutput(0));
         case "CDLDOJI":
            return core.cdldoji(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLDOJISTAR":
            return core.cdldojistar(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLDRAGONFLYDOJI":
            return core.cdldragonflydoji(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLENGULFING":
            return core.cdlengulfing(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLEVENINGDOJISTAR":
            return core.cdleveningdojistar(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.realOpt(0), h.intOutput(0));
         case "CDLEVENINGSTAR":
            return core.cdleveningstar(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.realOpt(0), h.intOutput(0));
         case "CDLGAPSIDESIDEWHITE":
            return core.cdlgapsidesidewhite(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLGRAVESTONEDOJI":
            return core.cdlgravestonedoji(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLHAMMER":
            return core.cdlhammer(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLHANGINGMAN":
            return core.cdlhangingman(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLHARAMI":
            return core.cdlharami(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLHARAMICROSS":
            return core.cdlharamicross(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLHIGHWAVE":
            return core.cdlhighwave(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLHIKKAKE":
            return core.cdlhikkake(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLHIKKAKEMOD":
            return core.cdlhikkakemod(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLHOMINGPIGEON":
            return core.cdlhomingpigeon(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLIDENTICAL3CROWS":
            return core.cdlidentical3crows(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLINNECK":
            return core.cdlinneck(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLINVERTEDHAMMER":
            return core.cdlinvertedhammer(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLKICKING":
            return core.cdlkicking(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLKICKINGBYLENGTH":
            return core.cdlkickingbylength(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLLADDERBOTTOM":
            return core.cdlladderbottom(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLLONGLEGGEDDOJI":
            return core.cdllongleggeddoji(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLLONGLINE":
            return core.cdllongline(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLMARUBOZU":
            return core.cdlmarubozu(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLMATCHINGLOW":
            return core.cdlmatchinglow(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLMATHOLD":
            return core.cdlmathold(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.realOpt(0), h.intOutput(0));
         case "CDLMORNINGDOJISTAR":
            return core.cdlmorningdojistar(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.realOpt(0), h.intOutput(0));
         case "CDLMORNINGSTAR":
            return core.cdlmorningstar(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.realOpt(0), h.intOutput(0));
         case "CDLONNECK":
            return core.cdlonneck(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLPIERCING":
            return core.cdlpiercing(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLRICKSHAWMAN":
            return core.cdlrickshawman(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLRISEFALL3METHODS":
            return core.cdlrisefall3methods(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLSEPARATINGLINES":
            return core.cdlseparatinglines(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLSHOOTINGSTAR":
            return core.cdlshootingstar(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLSHORTLINE":
            return core.cdlshortline(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLSPINNINGTOP":
            return core.cdlspinningtop(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLSTALLEDPATTERN":
            return core.cdlstalledpattern(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLSTICKSANDWICH":
            return core.cdlsticksandwich(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLTAKURI":
            return core.cdltakuri(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLTASUKIGAP":
            return core.cdltasukigap(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLTHRUSTING":
            return core.cdlthrusting(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLTRISTAR":
            return core.cdltristar(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLUNIQUE3RIVER":
            return core.cdlunique3river(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLUPSIDEGAP2CROWS":
            return core.cdlupsidegap2crows(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case "CDLXSIDEGAP3METHODS":
            return core.cdlxsidegap3methods(
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
         case "COPPOCK":
            return core.coppock(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.intOpt(1), h.intOpt(2), h.realOutput(0));
         case "CORREL":
            return core.correl(
               startIdx, endIdx, h.realInput(0), h.realInput(1), h.intOpt(0), h.realOutput(0));
         case "COS":
            return core.cos(
               startIdx, endIdx, h.realInput(0), h.realOutput(0));
         case "COSH":
            return core.cosh(
               startIdx, endIdx, h.realInput(0), h.realOutput(0));
         case "CTI":
            return core.cti(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case "CUMSUM":
            return core.cumsum(
               startIdx, endIdx, h.realInput(0), h.realOutput(0));
         case "CVI":
            return core.cvi(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.intOpt(0), h.intOpt(1), h.realOutput(0));
         case "DEMA":
            return core.dema(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case "DIV":
            return core.div(
               startIdx, endIdx, h.realInput(0), h.realInput(1), h.realOutput(0));
         case "DONCHIAN":
            return core.donchian(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.intOpt(0), h.realOutput(0), h.realOutput(1), h.realOutput(2));
         case "DPO":
            return core.dpo(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case "DX":
            return core.dx(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOpt(0), h.realOutput(0));
         case "EFI":
            return core.efi(
               startIdx, endIdx, h.price(0, 3), h.price(0, 4), h.intOpt(0), h.realOutput(0));
         case "EMA":
            return core.ema(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case "ER":
            return core.er(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case "ERI":
            return core.eri(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOpt(0), h.realOutput(0), h.realOutput(1));
         case "EXP":
            return core.exp(
               startIdx, endIdx, h.realInput(0), h.realOutput(0));
         case "FLOOR":
            return core.floor(
               startIdx, endIdx, h.realInput(0), h.realOutput(0));
         case "FOSC":
            return core.fosc(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case "FRACTAL":
            return core.fractal(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.intOpt(0), h.intOpt(1), h.intOutput(0), h.intOutput(1));
         case "HA":
            return core.ha(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.realOutput(0), h.realOutput(1), h.realOutput(2), h.realOutput(3));
         case "HMA":
            return core.hma(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case "HT_DCPERIOD":
            return core.htDcperiod(
               startIdx, endIdx, h.realInput(0), h.realOutput(0));
         case "HT_DCPHASE":
            return core.htDcphase(
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
            return core.htTrendmode(
               startIdx, endIdx, h.realInput(0), h.intOutput(0));
         case "IMI":
            return core.imi(
               startIdx, endIdx, h.price(0, 0), h.price(0, 3), h.intOpt(0), h.realOutput(0));
         case "KAMA":
            return core.kama(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case "KC":
            return core.kc(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOpt(0), h.intOpt(1), h.realOpt(2), h.realOutput(0), h.realOutput(1), h.realOutput(2));
         case "KDJ":
            return core.kdj(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOpt(0), h.intOpt(1), h.maTypeOpt(2), h.intOpt(3), h.maTypeOpt(4), h.realOutput(0), h.realOutput(1), h.realOutput(2));
         case "LINEARREG":
            return core.linearreg(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case "LINEARREG_ANGLE":
            return core.linearregAngle(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case "LINEARREG_INTERCEPT":
            return core.linearregIntercept(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case "LINEARREG_SLOPE":
            return core.linearregSlope(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case "LN":
            return core.ln(
               startIdx, endIdx, h.realInput(0), h.realOutput(0));
         case "LOG10":
            return core.log10(
               startIdx, endIdx, h.realInput(0), h.realOutput(0));
         case "MA":
            return core.ma(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.maTypeOpt(1), h.realOutput(0));
         case "MACD":
            return core.macd(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.intOpt(1), h.intOpt(2), h.realOutput(0), h.realOutput(1), h.realOutput(2));
         case "MACDEXT":
            return core.macdext(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.maTypeOpt(1), h.intOpt(2), h.maTypeOpt(3), h.intOpt(4), h.maTypeOpt(5), h.realOutput(0), h.realOutput(1), h.realOutput(2));
         case "MACDFIX":
            return core.macdfix(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0), h.realOutput(1), h.realOutput(2));
         case "MAMA":
            return core.mama(
               startIdx, endIdx, h.realInput(0), h.realOpt(0), h.realOpt(1), h.realOutput(0), h.realOutput(1));
         case "MARKETFI":
            return core.marketfi(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 4), h.realOutput(0));
         case "MASSI":
            return core.massi(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.intOpt(0), h.intOpt(1), h.realOutput(0));
         case "MAVP":
            return core.mavp(
               startIdx, endIdx, h.realInput(0), h.realInput(1), h.intOpt(0), h.intOpt(1), h.maTypeOpt(2), h.realOutput(0));
         case "MAX":
            return core.max(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case "MAXINDEX":
            return core.maxindex(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.intOutput(0));
         case "MEDPRICE":
            return core.medprice(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.realOutput(0));
         case "MFI":
            return core.mfi(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.price(0, 4), h.intOpt(0), h.realOutput(0));
         case "MIDPOINT":
            return core.midpoint(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case "MIDPRICE":
            return core.midprice(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.intOpt(0), h.realOutput(0));
         case "MIN":
            return core.min(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case "MININDEX":
            return core.minindex(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.intOutput(0));
         case "MINMAX":
            return core.minmax(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0), h.realOutput(1));
         case "MINMAXINDEX":
            return core.minmaxindex(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.intOutput(0), h.intOutput(1));
         case "MINUS_DI":
            return core.minusDi(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOpt(0), h.realOutput(0));
         case "MINUS_DM":
            return core.minusDm(
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
         case "PERCENTILE":
            return core.percentile(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOpt(1), h.realOutput(0));
         case "PERCENTRANK":
            return core.percentrank(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case "PLUS_DI":
            return core.plusDi(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOpt(0), h.realOutput(0));
         case "PLUS_DM":
            return core.plusDm(
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
         case "PVT":
            return core.pvt(
               startIdx, endIdx, h.price(0, 3), h.price(0, 4), h.realOutput(0));
         case "QSTICK":
            return core.qstick(
               startIdx, endIdx, h.price(0, 0), h.price(0, 3), h.intOpt(0), h.realOutput(0));
         case "RMA":
            return core.rma(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case "ROC":
            return core.roc(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case "ROCP":
            return core.rocp(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case "ROCR":
            return core.rocr(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case "ROCR100":
            return core.rocr100(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case "RSI":
            return core.rsi(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case "RVI":
            return core.rvi(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.intOpt(1), h.realOutput(0));
         case "RVOL":
            return core.rvol(
               startIdx, endIdx, h.price(0, 4), h.intOpt(0), h.realOutput(0));
         case "SAR":
            return core.sar(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.realOpt(0), h.realOpt(1), h.realOutput(0));
         case "SAREXT":
            return core.sarext(
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
         case "SMI":
            return core.smi(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOpt(0), h.intOpt(1), h.intOpt(2), h.intOpt(3), h.realOutput(0), h.realOutput(1));
         case "SQRT":
            return core.sqrt(
               startIdx, endIdx, h.realInput(0), h.realOutput(0));
         case "STDDEV":
            return core.stddev(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOpt(1), h.realOutput(0));
         case "STOCH":
            return core.stoch(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOpt(0), h.intOpt(1), h.maTypeOpt(2), h.intOpt(3), h.maTypeOpt(4), h.realOutput(0), h.realOutput(1));
         case "STOCHF":
            return core.stochf(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOpt(0), h.intOpt(1), h.maTypeOpt(2), h.realOutput(0), h.realOutput(1));
         case "STOCHRSI":
            return core.stochrsi(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.intOpt(1), h.intOpt(2), h.maTypeOpt(3), h.realOutput(0), h.realOutput(1));
         case "SUB":
            return core.sub(
               startIdx, endIdx, h.realInput(0), h.realInput(1), h.realOutput(0));
         case "SUM":
            return core.sum(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case "SUPERTREND":
            return core.supertrend(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOpt(0), h.realOpt(1), h.realOutput(0), h.intOutput(1));
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
            return core.trange(
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
         case "TSI":
            return core.tsi(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.intOpt(1), h.realOutput(0));
         case "TYPPRICE":
            return core.typprice(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.realOutput(0));
         case "ULTOSC":
            return core.ultosc(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOpt(0), h.intOpt(1), h.intOpt(2), h.realOutput(0));
         case "VAR":
            return core.var(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOpt(1), h.realOutput(0));
         case "VHF":
            return core.vhf(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case "VORTEX":
            return core.vortex(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOpt(0), h.realOutput(0), h.realOutput(1));
         case "VWAP":
            return core.vwap(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.price(0, 4), h.realOutput(0));
         case "VWMA":
            return core.vwma(
               startIdx, endIdx, h.realInput(0), h.price(1, 4), h.intOpt(0), h.realOutput(0));
         case "WAD":
            return core.wad(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.realOutput(0));
         case "WCLPRICE":
            return core.wclprice(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.realOutput(0));
         case "WILLR":
            return core.willr(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOpt(0), h.realOutput(0));
         case "WMA":
            return core.wma(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case "ZLEMA":
            return core.zlema(
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
         case "AC":
            return core.acLookback(h.intOpt(0), h.intOpt(1), h.intOpt(2));
         case "ACCBANDS":
            return core.accbandsLookback(h.intOpt(0));
         case "ACOS":
            return core.acosLookback();
         case "AD":
            return core.adLookback();
         case "ADD":
            return core.addLookback();
         case "ADOSC":
            return core.adoscLookback(h.intOpt(0), h.intOpt(1));
         case "ADR":
            return core.adrLookback(h.intOpt(0));
         case "ADX":
            return core.adxLookback(h.intOpt(0));
         case "ADXR":
            return core.adxrLookback(h.intOpt(0));
         case "AO":
            return core.aoLookback(h.intOpt(0), h.intOpt(1));
         case "APO":
            return core.apoLookback(h.intOpt(0), h.intOpt(1), h.maTypeOpt(2));
         case "AROON":
            return core.aroonLookback(h.intOpt(0));
         case "AROONOSC":
            return core.aroonoscLookback(h.intOpt(0));
         case "ASIN":
            return core.asinLookback();
         case "ATAN":
            return core.atanLookback();
         case "ATR":
            return core.atrLookback(h.intOpt(0));
         case "AVGDEV":
            return core.avgdevLookback(h.intOpt(0));
         case "AVGPRICE":
            return core.avgpriceLookback();
         case "BBANDS":
            return core.bbandsLookback(h.intOpt(0), h.realOpt(1), h.realOpt(2), h.maTypeOpt(3));
         case "BETA":
            return core.betaLookback(h.intOpt(0));
         case "BOP":
            return core.bopLookback();
         case "CCI":
            return core.cciLookback(h.intOpt(0));
         case "CDL2CROWS":
            return core.cdl2crowsLookback();
         case "CDL3BLACKCROWS":
            return core.cdl3blackcrowsLookback();
         case "CDL3INSIDE":
            return core.cdl3insideLookback();
         case "CDL3LINESTRIKE":
            return core.cdl3linestrikeLookback();
         case "CDL3OUTSIDE":
            return core.cdl3outsideLookback();
         case "CDL3STARSINSOUTH":
            return core.cdl3starsinsouthLookback();
         case "CDL3WHITESOLDIERS":
            return core.cdl3whitesoldiersLookback();
         case "CDLABANDONEDBABY":
            return core.cdlabandonedbabyLookback(h.realOpt(0));
         case "CDLADVANCEBLOCK":
            return core.cdladvanceblockLookback();
         case "CDLBELTHOLD":
            return core.cdlbeltholdLookback();
         case "CDLBREAKAWAY":
            return core.cdlbreakawayLookback();
         case "CDLCLOSINGMARUBOZU":
            return core.cdlclosingmarubozuLookback();
         case "CDLCONCEALBABYSWALL":
            return core.cdlconcealbabyswallLookback();
         case "CDLCOUNTERATTACK":
            return core.cdlcounterattackLookback();
         case "CDLDARKCLOUDCOVER":
            return core.cdldarkcloudcoverLookback(h.realOpt(0));
         case "CDLDOJI":
            return core.cdldojiLookback();
         case "CDLDOJISTAR":
            return core.cdldojistarLookback();
         case "CDLDRAGONFLYDOJI":
            return core.cdldragonflydojiLookback();
         case "CDLENGULFING":
            return core.cdlengulfingLookback();
         case "CDLEVENINGDOJISTAR":
            return core.cdleveningdojistarLookback(h.realOpt(0));
         case "CDLEVENINGSTAR":
            return core.cdleveningstarLookback(h.realOpt(0));
         case "CDLGAPSIDESIDEWHITE":
            return core.cdlgapsidesidewhiteLookback();
         case "CDLGRAVESTONEDOJI":
            return core.cdlgravestonedojiLookback();
         case "CDLHAMMER":
            return core.cdlhammerLookback();
         case "CDLHANGINGMAN":
            return core.cdlhangingmanLookback();
         case "CDLHARAMI":
            return core.cdlharamiLookback();
         case "CDLHARAMICROSS":
            return core.cdlharamicrossLookback();
         case "CDLHIGHWAVE":
            return core.cdlhighwaveLookback();
         case "CDLHIKKAKE":
            return core.cdlhikkakeLookback();
         case "CDLHIKKAKEMOD":
            return core.cdlhikkakemodLookback();
         case "CDLHOMINGPIGEON":
            return core.cdlhomingpigeonLookback();
         case "CDLIDENTICAL3CROWS":
            return core.cdlidentical3crowsLookback();
         case "CDLINNECK":
            return core.cdlinneckLookback();
         case "CDLINVERTEDHAMMER":
            return core.cdlinvertedhammerLookback();
         case "CDLKICKING":
            return core.cdlkickingLookback();
         case "CDLKICKINGBYLENGTH":
            return core.cdlkickingbylengthLookback();
         case "CDLLADDERBOTTOM":
            return core.cdlladderbottomLookback();
         case "CDLLONGLEGGEDDOJI":
            return core.cdllongleggeddojiLookback();
         case "CDLLONGLINE":
            return core.cdllonglineLookback();
         case "CDLMARUBOZU":
            return core.cdlmarubozuLookback();
         case "CDLMATCHINGLOW":
            return core.cdlmatchinglowLookback();
         case "CDLMATHOLD":
            return core.cdlmatholdLookback(h.realOpt(0));
         case "CDLMORNINGDOJISTAR":
            return core.cdlmorningdojistarLookback(h.realOpt(0));
         case "CDLMORNINGSTAR":
            return core.cdlmorningstarLookback(h.realOpt(0));
         case "CDLONNECK":
            return core.cdlonneckLookback();
         case "CDLPIERCING":
            return core.cdlpiercingLookback();
         case "CDLRICKSHAWMAN":
            return core.cdlrickshawmanLookback();
         case "CDLRISEFALL3METHODS":
            return core.cdlrisefall3methodsLookback();
         case "CDLSEPARATINGLINES":
            return core.cdlseparatinglinesLookback();
         case "CDLSHOOTINGSTAR":
            return core.cdlshootingstarLookback();
         case "CDLSHORTLINE":
            return core.cdlshortlineLookback();
         case "CDLSPINNINGTOP":
            return core.cdlspinningtopLookback();
         case "CDLSTALLEDPATTERN":
            return core.cdlstalledpatternLookback();
         case "CDLSTICKSANDWICH":
            return core.cdlsticksandwichLookback();
         case "CDLTAKURI":
            return core.cdltakuriLookback();
         case "CDLTASUKIGAP":
            return core.cdltasukigapLookback();
         case "CDLTHRUSTING":
            return core.cdlthrustingLookback();
         case "CDLTRISTAR":
            return core.cdltristarLookback();
         case "CDLUNIQUE3RIVER":
            return core.cdlunique3riverLookback();
         case "CDLUPSIDEGAP2CROWS":
            return core.cdlupsidegap2crowsLookback();
         case "CDLXSIDEGAP3METHODS":
            return core.cdlxsidegap3methodsLookback();
         case "CEIL":
            return core.ceilLookback();
         case "CMF":
            return core.cmfLookback(h.intOpt(0));
         case "CMO":
            return core.cmoLookback(h.intOpt(0));
         case "CMOU":
            return core.cmouLookback(h.intOpt(0));
         case "COPPOCK":
            return core.coppockLookback(h.intOpt(0), h.intOpt(1), h.intOpt(2));
         case "CORREL":
            return core.correlLookback(h.intOpt(0));
         case "COS":
            return core.cosLookback();
         case "COSH":
            return core.coshLookback();
         case "CTI":
            return core.ctiLookback(h.intOpt(0));
         case "CUMSUM":
            return core.cumsumLookback();
         case "CVI":
            return core.cviLookback(h.intOpt(0), h.intOpt(1));
         case "DEMA":
            return core.demaLookback(h.intOpt(0));
         case "DIV":
            return core.divLookback();
         case "DONCHIAN":
            return core.donchianLookback(h.intOpt(0));
         case "DPO":
            return core.dpoLookback(h.intOpt(0));
         case "DX":
            return core.dxLookback(h.intOpt(0));
         case "EFI":
            return core.efiLookback(h.intOpt(0));
         case "EMA":
            return core.emaLookback(h.intOpt(0));
         case "ER":
            return core.erLookback(h.intOpt(0));
         case "ERI":
            return core.eriLookback(h.intOpt(0));
         case "EXP":
            return core.expLookback();
         case "FLOOR":
            return core.floorLookback();
         case "FOSC":
            return core.foscLookback(h.intOpt(0));
         case "FRACTAL":
            return core.fractalLookback(h.intOpt(0), h.intOpt(1));
         case "HA":
            return core.haLookback();
         case "HMA":
            return core.hmaLookback(h.intOpt(0));
         case "HT_DCPERIOD":
            return core.htDcperiodLookback();
         case "HT_DCPHASE":
            return core.htDcphaseLookback();
         case "HT_PHASOR":
            return core.htPhasorLookback();
         case "HT_SINE":
            return core.htSineLookback();
         case "HT_TRENDLINE":
            return core.htTrendlineLookback();
         case "HT_TRENDMODE":
            return core.htTrendmodeLookback();
         case "IMI":
            return core.imiLookback(h.intOpt(0));
         case "KAMA":
            return core.kamaLookback(h.intOpt(0));
         case "KC":
            return core.kcLookback(h.intOpt(0), h.intOpt(1), h.realOpt(2));
         case "KDJ":
            return core.kdjLookback(h.intOpt(0), h.intOpt(1), h.maTypeOpt(2), h.intOpt(3), h.maTypeOpt(4));
         case "LINEARREG":
            return core.linearregLookback(h.intOpt(0));
         case "LINEARREG_ANGLE":
            return core.linearregAngleLookback(h.intOpt(0));
         case "LINEARREG_INTERCEPT":
            return core.linearregInterceptLookback(h.intOpt(0));
         case "LINEARREG_SLOPE":
            return core.linearregSlopeLookback(h.intOpt(0));
         case "LN":
            return core.lnLookback();
         case "LOG10":
            return core.log10Lookback();
         case "MA":
            return core.maLookback(h.intOpt(0), h.maTypeOpt(1));
         case "MACD":
            return core.macdLookback(h.intOpt(0), h.intOpt(1), h.intOpt(2));
         case "MACDEXT":
            return core.macdextLookback(h.intOpt(0), h.maTypeOpt(1), h.intOpt(2), h.maTypeOpt(3), h.intOpt(4), h.maTypeOpt(5));
         case "MACDFIX":
            return core.macdfixLookback(h.intOpt(0));
         case "MAMA":
            return core.mamaLookback(h.realOpt(0), h.realOpt(1));
         case "MARKETFI":
            return core.marketfiLookback();
         case "MASSI":
            return core.massiLookback(h.intOpt(0), h.intOpt(1));
         case "MAVP":
            return core.mavpLookback(h.intOpt(0), h.intOpt(1), h.maTypeOpt(2));
         case "MAX":
            return core.maxLookback(h.intOpt(0));
         case "MAXINDEX":
            return core.maxindexLookback(h.intOpt(0));
         case "MEDPRICE":
            return core.medpriceLookback();
         case "MFI":
            return core.mfiLookback(h.intOpt(0));
         case "MIDPOINT":
            return core.midpointLookback(h.intOpt(0));
         case "MIDPRICE":
            return core.midpriceLookback(h.intOpt(0));
         case "MIN":
            return core.minLookback(h.intOpt(0));
         case "MININDEX":
            return core.minindexLookback(h.intOpt(0));
         case "MINMAX":
            return core.minmaxLookback(h.intOpt(0));
         case "MINMAXINDEX":
            return core.minmaxindexLookback(h.intOpt(0));
         case "MINUS_DI":
            return core.minusDiLookback(h.intOpt(0));
         case "MINUS_DM":
            return core.minusDmLookback(h.intOpt(0));
         case "MOM":
            return core.momLookback(h.intOpt(0));
         case "MULT":
            return core.multLookback();
         case "NATR":
            return core.natrLookback(h.intOpt(0));
         case "NVI":
            return core.nviLookback();
         case "OBV":
            return core.obvLookback();
         case "PERCENTILE":
            return core.percentileLookback(h.intOpt(0), h.realOpt(1));
         case "PERCENTRANK":
            return core.percentrankLookback(h.intOpt(0));
         case "PLUS_DI":
            return core.plusDiLookback(h.intOpt(0));
         case "PLUS_DM":
            return core.plusDmLookback(h.intOpt(0));
         case "PPO":
            return core.ppoLookback(h.intOpt(0), h.intOpt(1), h.maTypeOpt(2));
         case "PVI":
            return core.pviLookback();
         case "PVO":
            return core.pvoLookback(h.intOpt(0), h.intOpt(1), h.maTypeOpt(2));
         case "PVT":
            return core.pvtLookback();
         case "QSTICK":
            return core.qstickLookback(h.intOpt(0));
         case "RMA":
            return core.rmaLookback(h.intOpt(0));
         case "ROC":
            return core.rocLookback(h.intOpt(0));
         case "ROCP":
            return core.rocpLookback(h.intOpt(0));
         case "ROCR":
            return core.rocrLookback(h.intOpt(0));
         case "ROCR100":
            return core.rocr100Lookback(h.intOpt(0));
         case "RSI":
            return core.rsiLookback(h.intOpt(0));
         case "RVI":
            return core.rviLookback(h.intOpt(0), h.intOpt(1));
         case "RVOL":
            return core.rvolLookback(h.intOpt(0));
         case "SAR":
            return core.sarLookback(h.realOpt(0), h.realOpt(1));
         case "SAREXT":
            return core.sarextLookback(h.realOpt(0), h.realOpt(1), h.realOpt(2), h.realOpt(3), h.realOpt(4), h.realOpt(5), h.realOpt(6), h.realOpt(7));
         case "SIN":
            return core.sinLookback();
         case "SINH":
            return core.sinhLookback();
         case "SMA":
            return core.smaLookback(h.intOpt(0));
         case "SMI":
            return core.smiLookback(h.intOpt(0), h.intOpt(1), h.intOpt(2), h.intOpt(3));
         case "SQRT":
            return core.sqrtLookback();
         case "STDDEV":
            return core.stddevLookback(h.intOpt(0), h.realOpt(1));
         case "STOCH":
            return core.stochLookback(h.intOpt(0), h.intOpt(1), h.maTypeOpt(2), h.intOpt(3), h.maTypeOpt(4));
         case "STOCHF":
            return core.stochfLookback(h.intOpt(0), h.intOpt(1), h.maTypeOpt(2));
         case "STOCHRSI":
            return core.stochrsiLookback(h.intOpt(0), h.intOpt(1), h.intOpt(2), h.maTypeOpt(3));
         case "SUB":
            return core.subLookback();
         case "SUM":
            return core.sumLookback(h.intOpt(0));
         case "SUPERTREND":
            return core.supertrendLookback(h.intOpt(0), h.realOpt(1));
         case "T3":
            return core.t3Lookback(h.intOpt(0), h.realOpt(1));
         case "TAN":
            return core.tanLookback();
         case "TANH":
            return core.tanhLookback();
         case "TEMA":
            return core.temaLookback(h.intOpt(0));
         case "TRANGE":
            return core.trangeLookback();
         case "TRIMA":
            return core.trimaLookback(h.intOpt(0));
         case "TRIX":
            return core.trixLookback(h.intOpt(0));
         case "TSF":
            return core.tsfLookback(h.intOpt(0));
         case "TSI":
            return core.tsiLookback(h.intOpt(0), h.intOpt(1));
         case "TYPPRICE":
            return core.typpriceLookback();
         case "ULTOSC":
            return core.ultoscLookback(h.intOpt(0), h.intOpt(1), h.intOpt(2));
         case "VAR":
            return core.varLookback(h.intOpt(0), h.realOpt(1));
         case "VHF":
            return core.vhfLookback(h.intOpt(0));
         case "VORTEX":
            return core.vortexLookback(h.intOpt(0));
         case "VWAP":
            return core.vwapLookback();
         case "VWMA":
            return core.vwmaLookback(h.intOpt(0));
         case "WAD":
            return core.wadLookback();
         case "WCLPRICE":
            return core.wclpriceLookback();
         case "WILLR":
            return core.willrLookback(h.intOpt(0));
         case "WMA":
            return core.wmaLookback(h.intOpt(0));
         case "ZLEMA":
            return core.zlemaLookback(h.intOpt(0));
         default:
            throw new IllegalArgumentException("no such function: " + h.info().name());
      }
   }
}
