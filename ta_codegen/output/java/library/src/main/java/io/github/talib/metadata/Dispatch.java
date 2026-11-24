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
import io.github.talib.RetCode;
import io.github.talib.TALibArgumentException;

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

   private static final String[] NAMES = {
      "AC", "ACCBANDS", "ACOS", "AD", "ADD", "ADOSC", "ADR", "ADX",
      "ADXR", "ALLIGATOR", "ALMA", "AO", "APO", "AROON", "AROONOSC", "ASI",
      "ASIN", "ATAN", "ATR", "AVGDEV", "AVGPRICE", "BBANDS", "BBW", "BETA",
      "BOP", "CCI", "CDL2CROWS", "CDL3BLACKCROWS", "CDL3INSIDE", "CDL3LINESTRIKE", "CDL3OUTSIDE", "CDL3STARSINSOUTH",
      "CDL3WHITESOLDIERS", "CDLABANDONEDBABY", "CDLADVANCEBLOCK", "CDLBELTHOLD", "CDLBREAKAWAY", "CDLCLOSINGMARUBOZU", "CDLCONCEALBABYSWALL", "CDLCOUNTERATTACK",
      "CDLDARKCLOUDCOVER", "CDLDOJI", "CDLDOJISTAR", "CDLDRAGONFLYDOJI", "CDLENGULFING", "CDLEVENINGDOJISTAR", "CDLEVENINGSTAR", "CDLGAPSIDESIDEWHITE",
      "CDLGRAVESTONEDOJI", "CDLHAMMER", "CDLHANGINGMAN", "CDLHARAMI", "CDLHARAMICROSS", "CDLHIGHWAVE", "CDLHIKKAKE", "CDLHIKKAKEMOD",
      "CDLHOMINGPIGEON", "CDLIDENTICAL3CROWS", "CDLINNECK", "CDLINVERTEDHAMMER", "CDLKICKING", "CDLKICKINGBYLENGTH", "CDLLADDERBOTTOM", "CDLLONGLEGGEDDOJI",
      "CDLLONGLINE", "CDLMARUBOZU", "CDLMATCHINGLOW", "CDLMATHOLD", "CDLMORNINGDOJISTAR", "CDLMORNINGSTAR", "CDLONNECK", "CDLPIERCING",
      "CDLRICKSHAWMAN", "CDLRISEFALL3METHODS", "CDLSEPARATINGLINES", "CDLSHOOTINGSTAR", "CDLSHORTLINE", "CDLSPINNINGTOP", "CDLSTALLEDPATTERN", "CDLSTICKSANDWICH",
      "CDLTAKURI", "CDLTASUKIGAP", "CDLTHRUSTING", "CDLTRISTAR", "CDLUNIQUE3RIVER", "CDLUPSIDEGAP2CROWS", "CDLXSIDEGAP3METHODS", "CE",
      "CEIL", "CG", "CHOP", "CHOPTR", "CKSP", "CMF", "CMO", "CMOU",
      "COPPOCK", "CORREL", "COS", "COSH", "CRSI", "CTI", "CUMSUM", "CVI",
      "DEMA", "DIV", "DONCHIAN", "DOSC", "DPO", "DX", "EFI", "EMA",
      "EMV", "ER", "ERI", "EXP", "FISHER", "FLOOR", "FOSC", "FRACTAL",
      "FRAMA", "GATOR", "HA", "HMA", "HT_DCPERIOD", "HT_DCPHASE", "HT_PHASOR", "HT_SINE",
      "HT_TRENDLINE", "HT_TRENDMODE", "IBS", "ICHIMOKU", "IMI", "KAMA", "KC", "KDJ",
      "KST", "KSTEXT", "KURTOSIS", "KVO", "LINEARREG", "LINEARREG_ANGLE", "LINEARREG_INTERCEPT", "LINEARREG_SLOPE",
      "LN", "LOG10", "MA", "MACD", "MACDEXT", "MACDFIX", "MACDV", "MAENV",
      "MAMA", "MARKETFI", "MASSI", "MAVP", "MAX", "MAXINDEX", "MCGD", "MEDIAN",
      "MEDPRICE", "MFI", "MIDPOINT", "MIDPRICE", "MIN", "MININDEX", "MINMAX", "MINMAXINDEX",
      "MINUS_DI", "MINUS_DM", "MOM", "MULT", "NATR", "NVI", "OBV", "PERCENTB",
      "PERCENTILE", "PERCENTRANK", "PLUS_DI", "PLUS_DM", "PPO", "PSO", "PVI", "PVO",
      "PVT", "QSTICK", "RMA", "ROC", "ROCP", "ROCR", "ROCR100", "ROGERSSATCHELL",
      "RSI", "RVI", "RVIR", "RVOL", "SAR", "SAREXT", "SHARPE", "SI",
      "SIN", "SINH", "SKEW", "SMA", "SMI", "SORTINO", "SQRT", "SQZMOM",
      "SSF", "STC", "STDDEV", "STOCH", "STOCHF", "STOCHRSI", "SUB", "SUM",
      "SUPERTREND", "SWAK_2PHP", "SWAK_BP", "SWAK_BUTTER", "SWAK_GAUSS", "SWAK_HP", "T3", "TAN",
      "TANH", "TEMA", "TRANGE", "TRIMA", "TRIX", "TSF", "TSI", "TYPPRICE",
      "ULTOSC", "VAR", "VHF", "VIDYA", "VORTEX", "VWAP", "VWMA", "WAD",
      "WAVETREND", "WCLPRICE", "WILLR", "WMA", "ZIGZAG", "ZLEMA",
   };

   /* A function's arm in every entry below is its index in NAMES. ParamHolder
      looks it up once, so a call switches on an int and never on a string. The
      lookup is open addressing on the cached hashCode(); a name from Functions
      is the interned literal in NAMES, so its equals() is an identity check. */
   private static final int MASK = 1023;
   private static final short[] ROUTES = routes();

   private static short[] routes() {
      short[] t = new short[MASK + 1];
      for (int r = 0; r < NAMES.length; r++) {
         int i = spread(NAMES[r].hashCode());
         while (t[i] != 0) {
            i = (i + 1) & MASK;
         }
         t[i] = (short) (r + 1);
      }
      return t;
   }

   private static int spread(int h) {
      return (h ^ (h >>> 16)) & MASK;
   }

   static int route(String name) {
      if (name == null) {
         return -1;
      }
      for (int i = spread(name.hashCode()); ; i = (i + 1) & MASK) {
         int r = ROUTES[i] - 1;
         if (r < 0 || NAMES[r].equals(name)) {
            return r;
         }
      }
   }

   static OutRange call(ParamHolder h, int startIdx, int endIdx) {
      switch (h.route() >>> 6) {
         case 0:
            return call0(h, startIdx, endIdx);
         case 1:
            return call1(h, startIdx, endIdx);
         case 2:
            return call2(h, startIdx, endIdx);
         case 3:
            return call3(h, startIdx, endIdx);
         default:
            throw new TALibArgumentException("no such function: " + h.info().name(), RetCode.INVALID_HANDLE);
      }
   }

   private static OutRange call0(ParamHolder h, int startIdx, int endIdx) {
      Core core = h.core();
      switch (h.route()) {
         case 0: // AC
            return core.ac(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.intOpt(0), h.intOpt(1), h.intOpt(2), h.realOutput(0));
         case 1: // ACCBANDS
            return core.accbands(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOpt(0), h.realOutput(0), h.realOutput(1), h.realOutput(2));
         case 2: // ACOS
            return core.acos(
               startIdx, endIdx, h.realInput(0), h.realOutput(0));
         case 3: // AD
            return core.ad(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.price(0, 4), h.realOutput(0));
         case 4: // ADD
            return core.add(
               startIdx, endIdx, h.realInput(0), h.realInput(1), h.realOutput(0));
         case 5: // ADOSC
            return core.adosc(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.price(0, 4), h.intOpt(0), h.intOpt(1), h.realOutput(0));
         case 6: // ADR
            return core.adr(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.intOpt(0), h.realOutput(0));
         case 7: // ADX
            return core.adx(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOpt(0), h.realOutput(0));
         case 8: // ADXR
            return core.adxr(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOpt(0), h.realOutput(0));
         case 9: // ALLIGATOR
            return core.alligator(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.intOpt(0), h.intOpt(1), h.intOpt(2), h.intOpt(3), h.intOpt(4), h.intOpt(5), h.realOutput(0), h.realOutput(1), h.realOutput(2));
         case 10: // ALMA
            return core.alma(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOpt(1), h.realOpt(2), h.realOutput(0));
         case 11: // AO
            return core.ao(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.intOpt(0), h.intOpt(1), h.realOutput(0));
         case 12: // APO
            return core.apo(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.intOpt(1), h.maTypeOpt(2), h.realOutput(0));
         case 13: // AROON
            return core.aroon(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.intOpt(0), h.realOutput(0), h.realOutput(1));
         case 14: // AROONOSC
            return core.aroonosc(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.intOpt(0), h.realOutput(0));
         case 15: // ASI
            return core.asi(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.realOpt(0), h.realOutput(0));
         case 16: // ASIN
            return core.asin(
               startIdx, endIdx, h.realInput(0), h.realOutput(0));
         case 17: // ATAN
            return core.atan(
               startIdx, endIdx, h.realInput(0), h.realOutput(0));
         case 18: // ATR
            return core.atr(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOpt(0), h.realOutput(0));
         case 19: // AVGDEV
            return core.avgdev(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case 20: // AVGPRICE
            return core.avgprice(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.realOutput(0));
         case 21: // BBANDS
            return core.bbands(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOpt(1), h.realOpt(2), h.maTypeOpt(3), h.realOutput(0), h.realOutput(1), h.realOutput(2));
         case 22: // BBW
            return core.bbw(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOpt(1), h.realOpt(2), h.maTypeOpt(3), h.realOutput(0));
         case 23: // BETA
            return core.beta(
               startIdx, endIdx, h.realInput(0), h.realInput(1), h.intOpt(0), h.realOutput(0));
         case 24: // BOP
            return core.bop(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.realOutput(0));
         case 25: // CCI
            return core.cci(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOpt(0), h.realOutput(0));
         case 26: // CDL2CROWS
            return core.cdl2crows(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case 27: // CDL3BLACKCROWS
            return core.cdl3blackcrows(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case 28: // CDL3INSIDE
            return core.cdl3inside(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case 29: // CDL3LINESTRIKE
            return core.cdl3linestrike(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case 30: // CDL3OUTSIDE
            return core.cdl3outside(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case 31: // CDL3STARSINSOUTH
            return core.cdl3starsinsouth(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case 32: // CDL3WHITESOLDIERS
            return core.cdl3whitesoldiers(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case 33: // CDLABANDONEDBABY
            return core.cdlabandonedbaby(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.realOpt(0), h.intOutput(0));
         case 34: // CDLADVANCEBLOCK
            return core.cdladvanceblock(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case 35: // CDLBELTHOLD
            return core.cdlbelthold(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case 36: // CDLBREAKAWAY
            return core.cdlbreakaway(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case 37: // CDLCLOSINGMARUBOZU
            return core.cdlclosingmarubozu(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case 38: // CDLCONCEALBABYSWALL
            return core.cdlconcealbabyswall(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case 39: // CDLCOUNTERATTACK
            return core.cdlcounterattack(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case 40: // CDLDARKCLOUDCOVER
            return core.cdldarkcloudcover(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.realOpt(0), h.intOutput(0));
         case 41: // CDLDOJI
            return core.cdldoji(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case 42: // CDLDOJISTAR
            return core.cdldojistar(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case 43: // CDLDRAGONFLYDOJI
            return core.cdldragonflydoji(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case 44: // CDLENGULFING
            return core.cdlengulfing(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case 45: // CDLEVENINGDOJISTAR
            return core.cdleveningdojistar(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.realOpt(0), h.intOutput(0));
         case 46: // CDLEVENINGSTAR
            return core.cdleveningstar(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.realOpt(0), h.intOutput(0));
         case 47: // CDLGAPSIDESIDEWHITE
            return core.cdlgapsidesidewhite(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case 48: // CDLGRAVESTONEDOJI
            return core.cdlgravestonedoji(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case 49: // CDLHAMMER
            return core.cdlhammer(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case 50: // CDLHANGINGMAN
            return core.cdlhangingman(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case 51: // CDLHARAMI
            return core.cdlharami(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case 52: // CDLHARAMICROSS
            return core.cdlharamicross(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case 53: // CDLHIGHWAVE
            return core.cdlhighwave(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case 54: // CDLHIKKAKE
            return core.cdlhikkake(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case 55: // CDLHIKKAKEMOD
            return core.cdlhikkakemod(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case 56: // CDLHOMINGPIGEON
            return core.cdlhomingpigeon(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case 57: // CDLIDENTICAL3CROWS
            return core.cdlidentical3crows(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case 58: // CDLINNECK
            return core.cdlinneck(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case 59: // CDLINVERTEDHAMMER
            return core.cdlinvertedhammer(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case 60: // CDLKICKING
            return core.cdlkicking(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case 61: // CDLKICKINGBYLENGTH
            return core.cdlkickingbylength(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case 62: // CDLLADDERBOTTOM
            return core.cdlladderbottom(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case 63: // CDLLONGLEGGEDDOJI
            return core.cdllongleggeddoji(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         default:
            throw new TALibArgumentException("no such function: " + h.info().name(), RetCode.INVALID_HANDLE);
      }
   }

   private static OutRange call1(ParamHolder h, int startIdx, int endIdx) {
      Core core = h.core();
      switch (h.route()) {
         case 64: // CDLLONGLINE
            return core.cdllongline(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case 65: // CDLMARUBOZU
            return core.cdlmarubozu(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case 66: // CDLMATCHINGLOW
            return core.cdlmatchinglow(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case 67: // CDLMATHOLD
            return core.cdlmathold(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.realOpt(0), h.intOutput(0));
         case 68: // CDLMORNINGDOJISTAR
            return core.cdlmorningdojistar(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.realOpt(0), h.intOutput(0));
         case 69: // CDLMORNINGSTAR
            return core.cdlmorningstar(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.realOpt(0), h.intOutput(0));
         case 70: // CDLONNECK
            return core.cdlonneck(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case 71: // CDLPIERCING
            return core.cdlpiercing(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case 72: // CDLRICKSHAWMAN
            return core.cdlrickshawman(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case 73: // CDLRISEFALL3METHODS
            return core.cdlrisefall3methods(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case 74: // CDLSEPARATINGLINES
            return core.cdlseparatinglines(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case 75: // CDLSHOOTINGSTAR
            return core.cdlshootingstar(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case 76: // CDLSHORTLINE
            return core.cdlshortline(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case 77: // CDLSPINNINGTOP
            return core.cdlspinningtop(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case 78: // CDLSTALLEDPATTERN
            return core.cdlstalledpattern(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case 79: // CDLSTICKSANDWICH
            return core.cdlsticksandwich(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case 80: // CDLTAKURI
            return core.cdltakuri(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case 81: // CDLTASUKIGAP
            return core.cdltasukigap(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case 82: // CDLTHRUSTING
            return core.cdlthrusting(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case 83: // CDLTRISTAR
            return core.cdltristar(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case 84: // CDLUNIQUE3RIVER
            return core.cdlunique3river(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case 85: // CDLUPSIDEGAP2CROWS
            return core.cdlupsidegap2crows(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case 86: // CDLXSIDEGAP3METHODS
            return core.cdlxsidegap3methods(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOutput(0));
         case 87: // CE
            return core.ce(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOpt(0), h.realOpt(1), h.realOutput(0), h.realOutput(1));
         case 88: // CEIL
            return core.ceil(
               startIdx, endIdx, h.realInput(0), h.realOutput(0));
         case 89: // CG
            return core.cg(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case 90: // CHOP
            return core.chop(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOpt(0), h.realOutput(0));
         case 91: // CHOPTR
            return core.choptr(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOpt(0), h.realOutput(0));
         case 92: // CKSP
            return core.cksp(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOpt(0), h.realOpt(1), h.intOpt(2), h.realOutput(0), h.realOutput(1));
         case 93: // CMF
            return core.cmf(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.price(0, 4), h.intOpt(0), h.realOutput(0));
         case 94: // CMO
            return core.cmo(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case 95: // CMOU
            return core.cmou(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case 96: // COPPOCK
            return core.coppock(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.intOpt(1), h.intOpt(2), h.realOutput(0));
         case 97: // CORREL
            return core.correl(
               startIdx, endIdx, h.realInput(0), h.realInput(1), h.intOpt(0), h.realOutput(0));
         case 98: // COS
            return core.cos(
               startIdx, endIdx, h.realInput(0), h.realOutput(0));
         case 99: // COSH
            return core.cosh(
               startIdx, endIdx, h.realInput(0), h.realOutput(0));
         case 100: // CRSI
            return core.crsi(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.intOpt(1), h.intOpt(2), h.realOutput(0));
         case 101: // CTI
            return core.cti(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case 102: // CUMSUM
            return core.cumsum(
               startIdx, endIdx, h.realInput(0), h.realOutput(0));
         case 103: // CVI
            return core.cvi(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.intOpt(0), h.intOpt(1), h.realOutput(0));
         case 104: // DEMA
            return core.dema(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case 105: // DIV
            return core.div(
               startIdx, endIdx, h.realInput(0), h.realInput(1), h.realOutput(0));
         case 106: // DONCHIAN
            return core.donchian(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.intOpt(0), h.realOutput(0), h.realOutput(1), h.realOutput(2));
         case 107: // DOSC
            return core.dosc(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.intOpt(1), h.intOpt(2), h.intOpt(3), h.realOutput(0));
         case 108: // DPO
            return core.dpo(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case 109: // DX
            return core.dx(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOpt(0), h.realOutput(0));
         case 110: // EFI
            return core.efi(
               startIdx, endIdx, h.price(0, 3), h.price(0, 4), h.intOpt(0), h.realOutput(0));
         case 111: // EMA
            return core.ema(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case 112: // EMV
            return core.emv(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 4), h.intOpt(0), h.realOpt(1), h.realOutput(0));
         case 113: // ER
            return core.er(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case 114: // ERI
            return core.eri(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOpt(0), h.realOutput(0), h.realOutput(1));
         case 115: // EXP
            return core.exp(
               startIdx, endIdx, h.realInput(0), h.realOutput(0));
         case 116: // FISHER
            return core.fisher(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.intOpt(0), h.realOutput(0), h.realOutput(1));
         case 117: // FLOOR
            return core.floor(
               startIdx, endIdx, h.realInput(0), h.realOutput(0));
         case 118: // FOSC
            return core.fosc(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case 119: // FRACTAL
            return core.fractal(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.intOpt(0), h.intOpt(1), h.intOutput(0), h.intOutput(1));
         case 120: // FRAMA
            return core.frama(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.intOpt(0), h.realOutput(0));
         case 121: // GATOR
            return core.gator(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.intOpt(0), h.intOpt(1), h.intOpt(2), h.intOpt(3), h.intOpt(4), h.intOpt(5), h.realOutput(0), h.realOutput(1));
         case 122: // HA
            return core.ha(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.realOutput(0), h.realOutput(1), h.realOutput(2), h.realOutput(3));
         case 123: // HMA
            return core.hma(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case 124: // HT_DCPERIOD
            return core.htDcperiod(
               startIdx, endIdx, h.realInput(0), h.realOutput(0));
         case 125: // HT_DCPHASE
            return core.htDcphase(
               startIdx, endIdx, h.realInput(0), h.realOutput(0));
         case 126: // HT_PHASOR
            return core.htPhasor(
               startIdx, endIdx, h.realInput(0), h.realOutput(0), h.realOutput(1));
         case 127: // HT_SINE
            return core.htSine(
               startIdx, endIdx, h.realInput(0), h.realOutput(0), h.realOutput(1));
         default:
            throw new TALibArgumentException("no such function: " + h.info().name(), RetCode.INVALID_HANDLE);
      }
   }

   private static OutRange call2(ParamHolder h, int startIdx, int endIdx) {
      Core core = h.core();
      switch (h.route()) {
         case 128: // HT_TRENDLINE
            return core.htTrendline(
               startIdx, endIdx, h.realInput(0), h.realOutput(0));
         case 129: // HT_TRENDMODE
            return core.htTrendmode(
               startIdx, endIdx, h.realInput(0), h.intOutput(0));
         case 130: // IBS
            return core.ibs(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.realOutput(0));
         case 131: // ICHIMOKU
            return core.ichimoku(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.intOpt(0), h.intOpt(1), h.intOpt(2), h.realOutput(0), h.realOutput(1), h.realOutput(2), h.realOutput(3));
         case 132: // IMI
            return core.imi(
               startIdx, endIdx, h.price(0, 0), h.price(0, 3), h.intOpt(0), h.realOutput(0));
         case 133: // KAMA
            return core.kama(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case 134: // KC
            return core.kc(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOpt(0), h.intOpt(1), h.realOpt(2), h.realOutput(0), h.realOutput(1), h.realOutput(2));
         case 135: // KDJ
            return core.kdj(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOpt(0), h.intOpt(1), h.maTypeOpt(2), h.intOpt(3), h.maTypeOpt(4), h.realOutput(0), h.realOutput(1), h.realOutput(2));
         case 136: // KST
            return core.kst(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.intOpt(1), h.intOpt(2), h.intOpt(3), h.intOpt(4), h.intOpt(5), h.intOpt(6), h.intOpt(7), h.intOpt(8), h.realOutput(0), h.realOutput(1));
         case 137: // KSTEXT
            return core.kstext(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.intOpt(1), h.intOpt(2), h.intOpt(3), h.intOpt(4), h.intOpt(5), h.intOpt(6), h.intOpt(7), h.intOpt(8), h.maTypeOpt(9), h.maTypeOpt(10), h.realOutput(0), h.realOutput(1));
         case 138: // KURTOSIS
            return core.kurtosis(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case 139: // KVO
            return core.kvo(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.price(0, 4), h.intOpt(0), h.intOpt(1), h.intOpt(2), h.realOutput(0), h.realOutput(1));
         case 140: // LINEARREG
            return core.linearreg(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case 141: // LINEARREG_ANGLE
            return core.linearregAngle(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case 142: // LINEARREG_INTERCEPT
            return core.linearregIntercept(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case 143: // LINEARREG_SLOPE
            return core.linearregSlope(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case 144: // LN
            return core.ln(
               startIdx, endIdx, h.realInput(0), h.realOutput(0));
         case 145: // LOG10
            return core.log10(
               startIdx, endIdx, h.realInput(0), h.realOutput(0));
         case 146: // MA
            return core.ma(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.maTypeOpt(1), h.realOutput(0));
         case 147: // MACD
            return core.macd(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.intOpt(1), h.intOpt(2), h.realOutput(0), h.realOutput(1), h.realOutput(2));
         case 148: // MACDEXT
            return core.macdext(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.maTypeOpt(1), h.intOpt(2), h.maTypeOpt(3), h.intOpt(4), h.maTypeOpt(5), h.realOutput(0), h.realOutput(1), h.realOutput(2));
         case 149: // MACDFIX
            return core.macdfix(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0), h.realOutput(1), h.realOutput(2));
         case 150: // MACDV
            return core.macdv(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOpt(0), h.intOpt(1), h.intOpt(2), h.realOutput(0), h.realOutput(1), h.realOutput(2));
         case 151: // MAENV
            return core.maenv(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOpt(1), h.realOpt(2), h.maTypeOpt(3), h.realOutput(0), h.realOutput(1), h.realOutput(2));
         case 152: // MAMA
            return core.mama(
               startIdx, endIdx, h.realInput(0), h.realOpt(0), h.realOpt(1), h.realOutput(0), h.realOutput(1));
         case 153: // MARKETFI
            return core.marketfi(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 4), h.realOutput(0));
         case 154: // MASSI
            return core.massi(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.intOpt(0), h.intOpt(1), h.realOutput(0));
         case 155: // MAVP
            return core.mavp(
               startIdx, endIdx, h.realInput(0), h.realInput(1), h.intOpt(0), h.intOpt(1), h.maTypeOpt(2), h.realOutput(0));
         case 156: // MAX
            return core.max(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case 157: // MAXINDEX
            return core.maxindex(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.intOutput(0));
         case 158: // MCGD
            return core.mcgd(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case 159: // MEDIAN
            return core.median(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case 160: // MEDPRICE
            return core.medprice(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.realOutput(0));
         case 161: // MFI
            return core.mfi(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.price(0, 4), h.intOpt(0), h.realOutput(0));
         case 162: // MIDPOINT
            return core.midpoint(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case 163: // MIDPRICE
            return core.midprice(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.intOpt(0), h.realOutput(0));
         case 164: // MIN
            return core.min(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case 165: // MININDEX
            return core.minindex(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.intOutput(0));
         case 166: // MINMAX
            return core.minmax(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0), h.realOutput(1));
         case 167: // MINMAXINDEX
            return core.minmaxindex(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.intOutput(0), h.intOutput(1));
         case 168: // MINUS_DI
            return core.minusDi(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOpt(0), h.realOutput(0));
         case 169: // MINUS_DM
            return core.minusDm(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.intOpt(0), h.realOutput(0));
         case 170: // MOM
            return core.mom(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case 171: // MULT
            return core.mult(
               startIdx, endIdx, h.realInput(0), h.realInput(1), h.realOutput(0));
         case 172: // NATR
            return core.natr(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOpt(0), h.realOutput(0));
         case 173: // NVI
            return core.nvi(
               startIdx, endIdx, h.price(0, 3), h.price(0, 4), h.realOutput(0));
         case 174: // OBV
            return core.obv(
               startIdx, endIdx, h.realInput(0), h.price(1, 4), h.realOutput(0));
         case 175: // PERCENTB
            return core.percentb(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOpt(1), h.realOpt(2), h.maTypeOpt(3), h.realOutput(0));
         case 176: // PERCENTILE
            return core.percentile(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOpt(1), h.realOutput(0));
         case 177: // PERCENTRANK
            return core.percentrank(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case 178: // PLUS_DI
            return core.plusDi(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOpt(0), h.realOutput(0));
         case 179: // PLUS_DM
            return core.plusDm(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.intOpt(0), h.realOutput(0));
         case 180: // PPO
            return core.ppo(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.intOpt(1), h.maTypeOpt(2), h.realOutput(0));
         case 181: // PSO
            return core.pso(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOpt(0), h.intOpt(1), h.realOutput(0));
         case 182: // PVI
            return core.pvi(
               startIdx, endIdx, h.price(0, 3), h.price(0, 4), h.realOutput(0));
         case 183: // PVO
            return core.pvo(
               startIdx, endIdx, h.price(0, 4), h.intOpt(0), h.intOpt(1), h.maTypeOpt(2), h.realOutput(0));
         case 184: // PVT
            return core.pvt(
               startIdx, endIdx, h.price(0, 3), h.price(0, 4), h.realOutput(0));
         case 185: // QSTICK
            return core.qstick(
               startIdx, endIdx, h.price(0, 0), h.price(0, 3), h.intOpt(0), h.realOutput(0));
         case 186: // RMA
            return core.rma(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case 187: // ROC
            return core.roc(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case 188: // ROCP
            return core.rocp(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case 189: // ROCR
            return core.rocr(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case 190: // ROCR100
            return core.rocr100(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case 191: // ROGERSSATCHELL
            return core.rogerssatchell(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOpt(0), h.realOpt(1), h.realOutput(0));
         default:
            throw new TALibArgumentException("no such function: " + h.info().name(), RetCode.INVALID_HANDLE);
      }
   }

   private static OutRange call3(ParamHolder h, int startIdx, int endIdx) {
      Core core = h.core();
      switch (h.route()) {
         case 192: // RSI
            return core.rsi(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case 193: // RVI
            return core.rvi(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.intOpt(1), h.realOutput(0));
         case 194: // RVIR
            return core.rvir(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.intOpt(0), h.intOpt(1), h.realOutput(0));
         case 195: // RVOL
            return core.rvol(
               startIdx, endIdx, h.price(0, 4), h.intOpt(0), h.realOutput(0));
         case 196: // SAR
            return core.sar(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.realOpt(0), h.realOpt(1), h.realOutput(0));
         case 197: // SAREXT
            return core.sarext(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.realOpt(0), h.realOpt(1), h.realOpt(2), h.realOpt(3), h.realOpt(4), h.realOpt(5), h.realOpt(6), h.realOpt(7), h.realOutput(0));
         case 198: // SHARPE
            return core.sharpe(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOpt(1), h.realOpt(2), h.realOutput(0));
         case 199: // SI
            return core.si(
               startIdx, endIdx, h.price(0, 0), h.price(0, 1), h.price(0, 2), h.price(0, 3), h.realOpt(0), h.realOutput(0));
         case 200: // SIN
            return core.sin(
               startIdx, endIdx, h.realInput(0), h.realOutput(0));
         case 201: // SINH
            return core.sinh(
               startIdx, endIdx, h.realInput(0), h.realOutput(0));
         case 202: // SKEW
            return core.skew(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case 203: // SMA
            return core.sma(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case 204: // SMI
            return core.smi(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOpt(0), h.intOpt(1), h.intOpt(2), h.intOpt(3), h.realOutput(0), h.realOutput(1));
         case 205: // SORTINO
            return core.sortino(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOpt(1), h.realOpt(2), h.realOutput(0));
         case 206: // SQRT
            return core.sqrt(
               startIdx, endIdx, h.realInput(0), h.realOutput(0));
         case 207: // SQZMOM
            return core.sqzmom(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOpt(0), h.realOpt(1), h.intOpt(2), h.realOpt(3), h.realOpt(4), h.realOpt(5), h.realOutput(0), h.intOutput(1));
         case 208: // SSF
            return core.ssf(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case 209: // STC
            return core.stc(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.intOpt(1), h.intOpt(2), h.realOutput(0));
         case 210: // STDDEV
            return core.stddev(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOpt(1), h.realOutput(0));
         case 211: // STOCH
            return core.stoch(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOpt(0), h.intOpt(1), h.maTypeOpt(2), h.intOpt(3), h.maTypeOpt(4), h.realOutput(0), h.realOutput(1));
         case 212: // STOCHF
            return core.stochf(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOpt(0), h.intOpt(1), h.maTypeOpt(2), h.realOutput(0), h.realOutput(1));
         case 213: // STOCHRSI
            return core.stochrsi(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.intOpt(1), h.intOpt(2), h.maTypeOpt(3), h.realOutput(0), h.realOutput(1));
         case 214: // SUB
            return core.sub(
               startIdx, endIdx, h.realInput(0), h.realInput(1), h.realOutput(0));
         case 215: // SUM
            return core.sum(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case 216: // SUPERTREND
            return core.supertrend(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOpt(0), h.realOpt(1), h.realOutput(0), h.intOutput(1));
         case 217: // SWAK_2PHP
            return core.swak2php(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case 218: // SWAK_BP
            return core.swakBp(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOpt(1), h.realOutput(0));
         case 219: // SWAK_BUTTER
            return core.swakButter(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case 220: // SWAK_GAUSS
            return core.swakGauss(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case 221: // SWAK_HP
            return core.swakHp(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case 222: // T3
            return core.t3(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOpt(1), h.realOutput(0));
         case 223: // TAN
            return core.tan(
               startIdx, endIdx, h.realInput(0), h.realOutput(0));
         case 224: // TANH
            return core.tanh(
               startIdx, endIdx, h.realInput(0), h.realOutput(0));
         case 225: // TEMA
            return core.tema(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case 226: // TRANGE
            return core.trange(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.realOutput(0));
         case 227: // TRIMA
            return core.trima(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case 228: // TRIX
            return core.trix(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case 229: // TSF
            return core.tsf(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case 230: // TSI
            return core.tsi(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.intOpt(1), h.realOutput(0));
         case 231: // TYPPRICE
            return core.typprice(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.realOutput(0));
         case 232: // ULTOSC
            return core.ultosc(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOpt(0), h.intOpt(1), h.intOpt(2), h.realOutput(0));
         case 233: // VAR
            return core.var(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOpt(1), h.realOutput(0));
         case 234: // VHF
            return core.vhf(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case 235: // VIDYA
            return core.vidya(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.intOpt(1), h.realOutput(0));
         case 236: // VORTEX
            return core.vortex(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOpt(0), h.realOutput(0), h.realOutput(1));
         case 237: // VWAP
            return core.vwap(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.price(0, 4), h.realOutput(0));
         case 238: // VWMA
            return core.vwma(
               startIdx, endIdx, h.realInput(0), h.price(1, 4), h.intOpt(0), h.realOutput(0));
         case 239: // WAD
            return core.wad(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.realOutput(0));
         case 240: // WAVETREND
            return core.wavetrend(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOpt(0), h.intOpt(1), h.intOpt(2), h.realOutput(0), h.realOutput(1));
         case 241: // WCLPRICE
            return core.wclprice(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.realOutput(0));
         case 242: // WILLR
            return core.willr(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.price(0, 3), h.intOpt(0), h.realOutput(0));
         case 243: // WMA
            return core.wma(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         case 244: // ZIGZAG
            return core.zigzag(
               startIdx, endIdx, h.price(0, 1), h.price(0, 2), h.realOpt(0), h.intOpt(1), h.realOutput(0), h.intOutput(1), h.intOutput(2));
         case 245: // ZLEMA
            return core.zlema(
               startIdx, endIdx, h.realInput(0), h.intOpt(0), h.realOutput(0));
         default:
            throw new TALibArgumentException("no such function: " + h.info().name(), RetCode.INVALID_HANDLE);
      }
   }

   /* The lookback tier. Separate from call() because it takes only the optional
      parameters -- no inputs, no outputs, no range -- so a caller can size its
      output arrays before binding them, exactly as TA_GetLookback allows. */
   static int lookback(ParamHolder h) {
      switch (h.route() >>> 6) {
         case 0:
            return lookback0(h);
         case 1:
            return lookback1(h);
         case 2:
            return lookback2(h);
         case 3:
            return lookback3(h);
         default:
            throw new TALibArgumentException("no such function: " + h.info().name(), RetCode.INVALID_HANDLE);
      }
   }

   private static int lookback0(ParamHolder h) {
      Core core = h.core();
      switch (h.route()) {
         case 0: // AC
            return core.acLookback(h.intOpt(0), h.intOpt(1), h.intOpt(2));
         case 1: // ACCBANDS
            return core.accbandsLookback(h.intOpt(0));
         case 2: // ACOS
            return core.acosLookback();
         case 3: // AD
            return core.adLookback();
         case 4: // ADD
            return core.addLookback();
         case 5: // ADOSC
            return core.adoscLookback(h.intOpt(0), h.intOpt(1));
         case 6: // ADR
            return core.adrLookback(h.intOpt(0));
         case 7: // ADX
            return core.adxLookback(h.intOpt(0));
         case 8: // ADXR
            return core.adxrLookback(h.intOpt(0));
         case 9: // ALLIGATOR
            return core.alligatorLookback(h.intOpt(0), h.intOpt(1), h.intOpt(2), h.intOpt(3), h.intOpt(4), h.intOpt(5));
         case 10: // ALMA
            return core.almaLookback(h.intOpt(0), h.realOpt(1), h.realOpt(2));
         case 11: // AO
            return core.aoLookback(h.intOpt(0), h.intOpt(1));
         case 12: // APO
            return core.apoLookback(h.intOpt(0), h.intOpt(1), h.maTypeOpt(2));
         case 13: // AROON
            return core.aroonLookback(h.intOpt(0));
         case 14: // AROONOSC
            return core.aroonoscLookback(h.intOpt(0));
         case 15: // ASI
            return core.asiLookback(h.realOpt(0));
         case 16: // ASIN
            return core.asinLookback();
         case 17: // ATAN
            return core.atanLookback();
         case 18: // ATR
            return core.atrLookback(h.intOpt(0));
         case 19: // AVGDEV
            return core.avgdevLookback(h.intOpt(0));
         case 20: // AVGPRICE
            return core.avgpriceLookback();
         case 21: // BBANDS
            return core.bbandsLookback(h.intOpt(0), h.realOpt(1), h.realOpt(2), h.maTypeOpt(3));
         case 22: // BBW
            return core.bbwLookback(h.intOpt(0), h.realOpt(1), h.realOpt(2), h.maTypeOpt(3));
         case 23: // BETA
            return core.betaLookback(h.intOpt(0));
         case 24: // BOP
            return core.bopLookback();
         case 25: // CCI
            return core.cciLookback(h.intOpt(0));
         case 26: // CDL2CROWS
            return core.cdl2crowsLookback();
         case 27: // CDL3BLACKCROWS
            return core.cdl3blackcrowsLookback();
         case 28: // CDL3INSIDE
            return core.cdl3insideLookback();
         case 29: // CDL3LINESTRIKE
            return core.cdl3linestrikeLookback();
         case 30: // CDL3OUTSIDE
            return core.cdl3outsideLookback();
         case 31: // CDL3STARSINSOUTH
            return core.cdl3starsinsouthLookback();
         case 32: // CDL3WHITESOLDIERS
            return core.cdl3whitesoldiersLookback();
         case 33: // CDLABANDONEDBABY
            return core.cdlabandonedbabyLookback(h.realOpt(0));
         case 34: // CDLADVANCEBLOCK
            return core.cdladvanceblockLookback();
         case 35: // CDLBELTHOLD
            return core.cdlbeltholdLookback();
         case 36: // CDLBREAKAWAY
            return core.cdlbreakawayLookback();
         case 37: // CDLCLOSINGMARUBOZU
            return core.cdlclosingmarubozuLookback();
         case 38: // CDLCONCEALBABYSWALL
            return core.cdlconcealbabyswallLookback();
         case 39: // CDLCOUNTERATTACK
            return core.cdlcounterattackLookback();
         case 40: // CDLDARKCLOUDCOVER
            return core.cdldarkcloudcoverLookback(h.realOpt(0));
         case 41: // CDLDOJI
            return core.cdldojiLookback();
         case 42: // CDLDOJISTAR
            return core.cdldojistarLookback();
         case 43: // CDLDRAGONFLYDOJI
            return core.cdldragonflydojiLookback();
         case 44: // CDLENGULFING
            return core.cdlengulfingLookback();
         case 45: // CDLEVENINGDOJISTAR
            return core.cdleveningdojistarLookback(h.realOpt(0));
         case 46: // CDLEVENINGSTAR
            return core.cdleveningstarLookback(h.realOpt(0));
         case 47: // CDLGAPSIDESIDEWHITE
            return core.cdlgapsidesidewhiteLookback();
         case 48: // CDLGRAVESTONEDOJI
            return core.cdlgravestonedojiLookback();
         case 49: // CDLHAMMER
            return core.cdlhammerLookback();
         case 50: // CDLHANGINGMAN
            return core.cdlhangingmanLookback();
         case 51: // CDLHARAMI
            return core.cdlharamiLookback();
         case 52: // CDLHARAMICROSS
            return core.cdlharamicrossLookback();
         case 53: // CDLHIGHWAVE
            return core.cdlhighwaveLookback();
         case 54: // CDLHIKKAKE
            return core.cdlhikkakeLookback();
         case 55: // CDLHIKKAKEMOD
            return core.cdlhikkakemodLookback();
         case 56: // CDLHOMINGPIGEON
            return core.cdlhomingpigeonLookback();
         case 57: // CDLIDENTICAL3CROWS
            return core.cdlidentical3crowsLookback();
         case 58: // CDLINNECK
            return core.cdlinneckLookback();
         case 59: // CDLINVERTEDHAMMER
            return core.cdlinvertedhammerLookback();
         case 60: // CDLKICKING
            return core.cdlkickingLookback();
         case 61: // CDLKICKINGBYLENGTH
            return core.cdlkickingbylengthLookback();
         case 62: // CDLLADDERBOTTOM
            return core.cdlladderbottomLookback();
         case 63: // CDLLONGLEGGEDDOJI
            return core.cdllongleggeddojiLookback();
         default:
            throw new TALibArgumentException("no such function: " + h.info().name(), RetCode.INVALID_HANDLE);
      }
   }

   private static int lookback1(ParamHolder h) {
      Core core = h.core();
      switch (h.route()) {
         case 64: // CDLLONGLINE
            return core.cdllonglineLookback();
         case 65: // CDLMARUBOZU
            return core.cdlmarubozuLookback();
         case 66: // CDLMATCHINGLOW
            return core.cdlmatchinglowLookback();
         case 67: // CDLMATHOLD
            return core.cdlmatholdLookback(h.realOpt(0));
         case 68: // CDLMORNINGDOJISTAR
            return core.cdlmorningdojistarLookback(h.realOpt(0));
         case 69: // CDLMORNINGSTAR
            return core.cdlmorningstarLookback(h.realOpt(0));
         case 70: // CDLONNECK
            return core.cdlonneckLookback();
         case 71: // CDLPIERCING
            return core.cdlpiercingLookback();
         case 72: // CDLRICKSHAWMAN
            return core.cdlrickshawmanLookback();
         case 73: // CDLRISEFALL3METHODS
            return core.cdlrisefall3methodsLookback();
         case 74: // CDLSEPARATINGLINES
            return core.cdlseparatinglinesLookback();
         case 75: // CDLSHOOTINGSTAR
            return core.cdlshootingstarLookback();
         case 76: // CDLSHORTLINE
            return core.cdlshortlineLookback();
         case 77: // CDLSPINNINGTOP
            return core.cdlspinningtopLookback();
         case 78: // CDLSTALLEDPATTERN
            return core.cdlstalledpatternLookback();
         case 79: // CDLSTICKSANDWICH
            return core.cdlsticksandwichLookback();
         case 80: // CDLTAKURI
            return core.cdltakuriLookback();
         case 81: // CDLTASUKIGAP
            return core.cdltasukigapLookback();
         case 82: // CDLTHRUSTING
            return core.cdlthrustingLookback();
         case 83: // CDLTRISTAR
            return core.cdltristarLookback();
         case 84: // CDLUNIQUE3RIVER
            return core.cdlunique3riverLookback();
         case 85: // CDLUPSIDEGAP2CROWS
            return core.cdlupsidegap2crowsLookback();
         case 86: // CDLXSIDEGAP3METHODS
            return core.cdlxsidegap3methodsLookback();
         case 87: // CE
            return core.ceLookback(h.intOpt(0), h.realOpt(1));
         case 88: // CEIL
            return core.ceilLookback();
         case 89: // CG
            return core.cgLookback(h.intOpt(0));
         case 90: // CHOP
            return core.chopLookback(h.intOpt(0));
         case 91: // CHOPTR
            return core.choptrLookback(h.intOpt(0));
         case 92: // CKSP
            return core.ckspLookback(h.intOpt(0), h.realOpt(1), h.intOpt(2));
         case 93: // CMF
            return core.cmfLookback(h.intOpt(0));
         case 94: // CMO
            return core.cmoLookback(h.intOpt(0));
         case 95: // CMOU
            return core.cmouLookback(h.intOpt(0));
         case 96: // COPPOCK
            return core.coppockLookback(h.intOpt(0), h.intOpt(1), h.intOpt(2));
         case 97: // CORREL
            return core.correlLookback(h.intOpt(0));
         case 98: // COS
            return core.cosLookback();
         case 99: // COSH
            return core.coshLookback();
         case 100: // CRSI
            return core.crsiLookback(h.intOpt(0), h.intOpt(1), h.intOpt(2));
         case 101: // CTI
            return core.ctiLookback(h.intOpt(0));
         case 102: // CUMSUM
            return core.cumsumLookback();
         case 103: // CVI
            return core.cviLookback(h.intOpt(0), h.intOpt(1));
         case 104: // DEMA
            return core.demaLookback(h.intOpt(0));
         case 105: // DIV
            return core.divLookback();
         case 106: // DONCHIAN
            return core.donchianLookback(h.intOpt(0));
         case 107: // DOSC
            return core.doscLookback(h.intOpt(0), h.intOpt(1), h.intOpt(2), h.intOpt(3));
         case 108: // DPO
            return core.dpoLookback(h.intOpt(0));
         case 109: // DX
            return core.dxLookback(h.intOpt(0));
         case 110: // EFI
            return core.efiLookback(h.intOpt(0));
         case 111: // EMA
            return core.emaLookback(h.intOpt(0));
         case 112: // EMV
            return core.emvLookback(h.intOpt(0), h.realOpt(1));
         case 113: // ER
            return core.erLookback(h.intOpt(0));
         case 114: // ERI
            return core.eriLookback(h.intOpt(0));
         case 115: // EXP
            return core.expLookback();
         case 116: // FISHER
            return core.fisherLookback(h.intOpt(0));
         case 117: // FLOOR
            return core.floorLookback();
         case 118: // FOSC
            return core.foscLookback(h.intOpt(0));
         case 119: // FRACTAL
            return core.fractalLookback(h.intOpt(0), h.intOpt(1));
         case 120: // FRAMA
            return core.framaLookback(h.intOpt(0));
         case 121: // GATOR
            return core.gatorLookback(h.intOpt(0), h.intOpt(1), h.intOpt(2), h.intOpt(3), h.intOpt(4), h.intOpt(5));
         case 122: // HA
            return core.haLookback();
         case 123: // HMA
            return core.hmaLookback(h.intOpt(0));
         case 124: // HT_DCPERIOD
            return core.htDcperiodLookback();
         case 125: // HT_DCPHASE
            return core.htDcphaseLookback();
         case 126: // HT_PHASOR
            return core.htPhasorLookback();
         case 127: // HT_SINE
            return core.htSineLookback();
         default:
            throw new TALibArgumentException("no such function: " + h.info().name(), RetCode.INVALID_HANDLE);
      }
   }

   private static int lookback2(ParamHolder h) {
      Core core = h.core();
      switch (h.route()) {
         case 128: // HT_TRENDLINE
            return core.htTrendlineLookback();
         case 129: // HT_TRENDMODE
            return core.htTrendmodeLookback();
         case 130: // IBS
            return core.ibsLookback();
         case 131: // ICHIMOKU
            return core.ichimokuLookback(h.intOpt(0), h.intOpt(1), h.intOpt(2));
         case 132: // IMI
            return core.imiLookback(h.intOpt(0));
         case 133: // KAMA
            return core.kamaLookback(h.intOpt(0));
         case 134: // KC
            return core.kcLookback(h.intOpt(0), h.intOpt(1), h.realOpt(2));
         case 135: // KDJ
            return core.kdjLookback(h.intOpt(0), h.intOpt(1), h.maTypeOpt(2), h.intOpt(3), h.maTypeOpt(4));
         case 136: // KST
            return core.kstLookback(h.intOpt(0), h.intOpt(1), h.intOpt(2), h.intOpt(3), h.intOpt(4), h.intOpt(5), h.intOpt(6), h.intOpt(7), h.intOpt(8));
         case 137: // KSTEXT
            return core.kstextLookback(h.intOpt(0), h.intOpt(1), h.intOpt(2), h.intOpt(3), h.intOpt(4), h.intOpt(5), h.intOpt(6), h.intOpt(7), h.intOpt(8), h.maTypeOpt(9), h.maTypeOpt(10));
         case 138: // KURTOSIS
            return core.kurtosisLookback(h.intOpt(0));
         case 139: // KVO
            return core.kvoLookback(h.intOpt(0), h.intOpt(1), h.intOpt(2));
         case 140: // LINEARREG
            return core.linearregLookback(h.intOpt(0));
         case 141: // LINEARREG_ANGLE
            return core.linearregAngleLookback(h.intOpt(0));
         case 142: // LINEARREG_INTERCEPT
            return core.linearregInterceptLookback(h.intOpt(0));
         case 143: // LINEARREG_SLOPE
            return core.linearregSlopeLookback(h.intOpt(0));
         case 144: // LN
            return core.lnLookback();
         case 145: // LOG10
            return core.log10Lookback();
         case 146: // MA
            return core.maLookback(h.intOpt(0), h.maTypeOpt(1));
         case 147: // MACD
            return core.macdLookback(h.intOpt(0), h.intOpt(1), h.intOpt(2));
         case 148: // MACDEXT
            return core.macdextLookback(h.intOpt(0), h.maTypeOpt(1), h.intOpt(2), h.maTypeOpt(3), h.intOpt(4), h.maTypeOpt(5));
         case 149: // MACDFIX
            return core.macdfixLookback(h.intOpt(0));
         case 150: // MACDV
            return core.macdvLookback(h.intOpt(0), h.intOpt(1), h.intOpt(2));
         case 151: // MAENV
            return core.maenvLookback(h.intOpt(0), h.realOpt(1), h.realOpt(2), h.maTypeOpt(3));
         case 152: // MAMA
            return core.mamaLookback(h.realOpt(0), h.realOpt(1));
         case 153: // MARKETFI
            return core.marketfiLookback();
         case 154: // MASSI
            return core.massiLookback(h.intOpt(0), h.intOpt(1));
         case 155: // MAVP
            return core.mavpLookback(h.intOpt(0), h.intOpt(1), h.maTypeOpt(2));
         case 156: // MAX
            return core.maxLookback(h.intOpt(0));
         case 157: // MAXINDEX
            return core.maxindexLookback(h.intOpt(0));
         case 158: // MCGD
            return core.mcgdLookback(h.intOpt(0));
         case 159: // MEDIAN
            return core.medianLookback(h.intOpt(0));
         case 160: // MEDPRICE
            return core.medpriceLookback();
         case 161: // MFI
            return core.mfiLookback(h.intOpt(0));
         case 162: // MIDPOINT
            return core.midpointLookback(h.intOpt(0));
         case 163: // MIDPRICE
            return core.midpriceLookback(h.intOpt(0));
         case 164: // MIN
            return core.minLookback(h.intOpt(0));
         case 165: // MININDEX
            return core.minindexLookback(h.intOpt(0));
         case 166: // MINMAX
            return core.minmaxLookback(h.intOpt(0));
         case 167: // MINMAXINDEX
            return core.minmaxindexLookback(h.intOpt(0));
         case 168: // MINUS_DI
            return core.minusDiLookback(h.intOpt(0));
         case 169: // MINUS_DM
            return core.minusDmLookback(h.intOpt(0));
         case 170: // MOM
            return core.momLookback(h.intOpt(0));
         case 171: // MULT
            return core.multLookback();
         case 172: // NATR
            return core.natrLookback(h.intOpt(0));
         case 173: // NVI
            return core.nviLookback();
         case 174: // OBV
            return core.obvLookback();
         case 175: // PERCENTB
            return core.percentbLookback(h.intOpt(0), h.realOpt(1), h.realOpt(2), h.maTypeOpt(3));
         case 176: // PERCENTILE
            return core.percentileLookback(h.intOpt(0), h.realOpt(1));
         case 177: // PERCENTRANK
            return core.percentrankLookback(h.intOpt(0));
         case 178: // PLUS_DI
            return core.plusDiLookback(h.intOpt(0));
         case 179: // PLUS_DM
            return core.plusDmLookback(h.intOpt(0));
         case 180: // PPO
            return core.ppoLookback(h.intOpt(0), h.intOpt(1), h.maTypeOpt(2));
         case 181: // PSO
            return core.psoLookback(h.intOpt(0), h.intOpt(1));
         case 182: // PVI
            return core.pviLookback();
         case 183: // PVO
            return core.pvoLookback(h.intOpt(0), h.intOpt(1), h.maTypeOpt(2));
         case 184: // PVT
            return core.pvtLookback();
         case 185: // QSTICK
            return core.qstickLookback(h.intOpt(0));
         case 186: // RMA
            return core.rmaLookback(h.intOpt(0));
         case 187: // ROC
            return core.rocLookback(h.intOpt(0));
         case 188: // ROCP
            return core.rocpLookback(h.intOpt(0));
         case 189: // ROCR
            return core.rocrLookback(h.intOpt(0));
         case 190: // ROCR100
            return core.rocr100Lookback(h.intOpt(0));
         case 191: // ROGERSSATCHELL
            return core.rogerssatchellLookback(h.intOpt(0), h.realOpt(1));
         default:
            throw new TALibArgumentException("no such function: " + h.info().name(), RetCode.INVALID_HANDLE);
      }
   }

   private static int lookback3(ParamHolder h) {
      Core core = h.core();
      switch (h.route()) {
         case 192: // RSI
            return core.rsiLookback(h.intOpt(0));
         case 193: // RVI
            return core.rviLookback(h.intOpt(0), h.intOpt(1));
         case 194: // RVIR
            return core.rvirLookback(h.intOpt(0), h.intOpt(1));
         case 195: // RVOL
            return core.rvolLookback(h.intOpt(0));
         case 196: // SAR
            return core.sarLookback(h.realOpt(0), h.realOpt(1));
         case 197: // SAREXT
            return core.sarextLookback(h.realOpt(0), h.realOpt(1), h.realOpt(2), h.realOpt(3), h.realOpt(4), h.realOpt(5), h.realOpt(6), h.realOpt(7));
         case 198: // SHARPE
            return core.sharpeLookback(h.intOpt(0), h.realOpt(1), h.realOpt(2));
         case 199: // SI
            return core.siLookback(h.realOpt(0));
         case 200: // SIN
            return core.sinLookback();
         case 201: // SINH
            return core.sinhLookback();
         case 202: // SKEW
            return core.skewLookback(h.intOpt(0));
         case 203: // SMA
            return core.smaLookback(h.intOpt(0));
         case 204: // SMI
            return core.smiLookback(h.intOpt(0), h.intOpt(1), h.intOpt(2), h.intOpt(3));
         case 205: // SORTINO
            return core.sortinoLookback(h.intOpt(0), h.realOpt(1), h.realOpt(2));
         case 206: // SQRT
            return core.sqrtLookback();
         case 207: // SQZMOM
            return core.sqzmomLookback(h.intOpt(0), h.realOpt(1), h.intOpt(2), h.realOpt(3), h.realOpt(4), h.realOpt(5));
         case 208: // SSF
            return core.ssfLookback(h.intOpt(0));
         case 209: // STC
            return core.stcLookback(h.intOpt(0), h.intOpt(1), h.intOpt(2));
         case 210: // STDDEV
            return core.stddevLookback(h.intOpt(0), h.realOpt(1));
         case 211: // STOCH
            return core.stochLookback(h.intOpt(0), h.intOpt(1), h.maTypeOpt(2), h.intOpt(3), h.maTypeOpt(4));
         case 212: // STOCHF
            return core.stochfLookback(h.intOpt(0), h.intOpt(1), h.maTypeOpt(2));
         case 213: // STOCHRSI
            return core.stochrsiLookback(h.intOpt(0), h.intOpt(1), h.intOpt(2), h.maTypeOpt(3));
         case 214: // SUB
            return core.subLookback();
         case 215: // SUM
            return core.sumLookback(h.intOpt(0));
         case 216: // SUPERTREND
            return core.supertrendLookback(h.intOpt(0), h.realOpt(1));
         case 217: // SWAK_2PHP
            return core.swak2phpLookback(h.intOpt(0));
         case 218: // SWAK_BP
            return core.swakBpLookback(h.intOpt(0), h.realOpt(1));
         case 219: // SWAK_BUTTER
            return core.swakButterLookback(h.intOpt(0));
         case 220: // SWAK_GAUSS
            return core.swakGaussLookback(h.intOpt(0));
         case 221: // SWAK_HP
            return core.swakHpLookback(h.intOpt(0));
         case 222: // T3
            return core.t3Lookback(h.intOpt(0), h.realOpt(1));
         case 223: // TAN
            return core.tanLookback();
         case 224: // TANH
            return core.tanhLookback();
         case 225: // TEMA
            return core.temaLookback(h.intOpt(0));
         case 226: // TRANGE
            return core.trangeLookback();
         case 227: // TRIMA
            return core.trimaLookback(h.intOpt(0));
         case 228: // TRIX
            return core.trixLookback(h.intOpt(0));
         case 229: // TSF
            return core.tsfLookback(h.intOpt(0));
         case 230: // TSI
            return core.tsiLookback(h.intOpt(0), h.intOpt(1));
         case 231: // TYPPRICE
            return core.typpriceLookback();
         case 232: // ULTOSC
            return core.ultoscLookback(h.intOpt(0), h.intOpt(1), h.intOpt(2));
         case 233: // VAR
            return core.varLookback(h.intOpt(0), h.realOpt(1));
         case 234: // VHF
            return core.vhfLookback(h.intOpt(0));
         case 235: // VIDYA
            return core.vidyaLookback(h.intOpt(0), h.intOpt(1));
         case 236: // VORTEX
            return core.vortexLookback(h.intOpt(0));
         case 237: // VWAP
            return core.vwapLookback();
         case 238: // VWMA
            return core.vwmaLookback(h.intOpt(0));
         case 239: // WAD
            return core.wadLookback();
         case 240: // WAVETREND
            return core.wavetrendLookback(h.intOpt(0), h.intOpt(1), h.intOpt(2));
         case 241: // WCLPRICE
            return core.wclpriceLookback();
         case 242: // WILLR
            return core.willrLookback(h.intOpt(0));
         case 243: // WMA
            return core.wmaLookback(h.intOpt(0));
         case 244: // ZIGZAG
            return core.zigzagLookback(h.realOpt(0), h.intOpt(1));
         case 245: // ZLEMA
            return core.zlemaLookback(h.intOpt(0));
         default:
            throw new TALibArgumentException("no such function: " + h.info().name(), RetCode.INVALID_HANDLE);
      }
   }

   static int displayShift(ParamHolder h, int outputIdx) {
      switch (h.route() >>> 6) {
         case 0:
            return displayShift0(h, outputIdx);
         case 1:
            return displayShift1(h, outputIdx);
         case 2:
            return displayShift2(h, outputIdx);
         case 3:
            return displayShift3(h, outputIdx);
         default:
            throw new TALibArgumentException("no such function: " + h.info().name(), RetCode.INVALID_HANDLE);
      }
   }

   private static int displayShift0(ParamHolder h, int outputIdx) {
      Core core = h.core();
      switch (h.route()) {
         case 0: // AC
            return core.acDisplayShift(h.intOpt(0), h.intOpt(1), h.intOpt(2), outputIdx);
         case 1: // ACCBANDS
            return core.accbandsDisplayShift(h.intOpt(0), outputIdx);
         case 2: // ACOS
            return core.acosDisplayShift(outputIdx);
         case 3: // AD
            return core.adDisplayShift(outputIdx);
         case 4: // ADD
            return core.addDisplayShift(outputIdx);
         case 5: // ADOSC
            return core.adoscDisplayShift(h.intOpt(0), h.intOpt(1), outputIdx);
         case 6: // ADR
            return core.adrDisplayShift(h.intOpt(0), outputIdx);
         case 7: // ADX
            return core.adxDisplayShift(h.intOpt(0), outputIdx);
         case 8: // ADXR
            return core.adxrDisplayShift(h.intOpt(0), outputIdx);
         case 9: // ALLIGATOR
            return core.alligatorDisplayShift(h.intOpt(0), h.intOpt(1), h.intOpt(2), h.intOpt(3), h.intOpt(4), h.intOpt(5), outputIdx);
         case 10: // ALMA
            return core.almaDisplayShift(h.intOpt(0), h.realOpt(1), h.realOpt(2), outputIdx);
         case 11: // AO
            return core.aoDisplayShift(h.intOpt(0), h.intOpt(1), outputIdx);
         case 12: // APO
            return core.apoDisplayShift(h.intOpt(0), h.intOpt(1), h.maTypeOpt(2), outputIdx);
         case 13: // AROON
            return core.aroonDisplayShift(h.intOpt(0), outputIdx);
         case 14: // AROONOSC
            return core.aroonoscDisplayShift(h.intOpt(0), outputIdx);
         case 15: // ASI
            return core.asiDisplayShift(h.realOpt(0), outputIdx);
         case 16: // ASIN
            return core.asinDisplayShift(outputIdx);
         case 17: // ATAN
            return core.atanDisplayShift(outputIdx);
         case 18: // ATR
            return core.atrDisplayShift(h.intOpt(0), outputIdx);
         case 19: // AVGDEV
            return core.avgdevDisplayShift(h.intOpt(0), outputIdx);
         case 20: // AVGPRICE
            return core.avgpriceDisplayShift(outputIdx);
         case 21: // BBANDS
            return core.bbandsDisplayShift(h.intOpt(0), h.realOpt(1), h.realOpt(2), h.maTypeOpt(3), outputIdx);
         case 22: // BBW
            return core.bbwDisplayShift(h.intOpt(0), h.realOpt(1), h.realOpt(2), h.maTypeOpt(3), outputIdx);
         case 23: // BETA
            return core.betaDisplayShift(h.intOpt(0), outputIdx);
         case 24: // BOP
            return core.bopDisplayShift(outputIdx);
         case 25: // CCI
            return core.cciDisplayShift(h.intOpt(0), outputIdx);
         case 26: // CDL2CROWS
            return core.cdl2crowsDisplayShift(outputIdx);
         case 27: // CDL3BLACKCROWS
            return core.cdl3blackcrowsDisplayShift(outputIdx);
         case 28: // CDL3INSIDE
            return core.cdl3insideDisplayShift(outputIdx);
         case 29: // CDL3LINESTRIKE
            return core.cdl3linestrikeDisplayShift(outputIdx);
         case 30: // CDL3OUTSIDE
            return core.cdl3outsideDisplayShift(outputIdx);
         case 31: // CDL3STARSINSOUTH
            return core.cdl3starsinsouthDisplayShift(outputIdx);
         case 32: // CDL3WHITESOLDIERS
            return core.cdl3whitesoldiersDisplayShift(outputIdx);
         case 33: // CDLABANDONEDBABY
            return core.cdlabandonedbabyDisplayShift(h.realOpt(0), outputIdx);
         case 34: // CDLADVANCEBLOCK
            return core.cdladvanceblockDisplayShift(outputIdx);
         case 35: // CDLBELTHOLD
            return core.cdlbeltholdDisplayShift(outputIdx);
         case 36: // CDLBREAKAWAY
            return core.cdlbreakawayDisplayShift(outputIdx);
         case 37: // CDLCLOSINGMARUBOZU
            return core.cdlclosingmarubozuDisplayShift(outputIdx);
         case 38: // CDLCONCEALBABYSWALL
            return core.cdlconcealbabyswallDisplayShift(outputIdx);
         case 39: // CDLCOUNTERATTACK
            return core.cdlcounterattackDisplayShift(outputIdx);
         case 40: // CDLDARKCLOUDCOVER
            return core.cdldarkcloudcoverDisplayShift(h.realOpt(0), outputIdx);
         case 41: // CDLDOJI
            return core.cdldojiDisplayShift(outputIdx);
         case 42: // CDLDOJISTAR
            return core.cdldojistarDisplayShift(outputIdx);
         case 43: // CDLDRAGONFLYDOJI
            return core.cdldragonflydojiDisplayShift(outputIdx);
         case 44: // CDLENGULFING
            return core.cdlengulfingDisplayShift(outputIdx);
         case 45: // CDLEVENINGDOJISTAR
            return core.cdleveningdojistarDisplayShift(h.realOpt(0), outputIdx);
         case 46: // CDLEVENINGSTAR
            return core.cdleveningstarDisplayShift(h.realOpt(0), outputIdx);
         case 47: // CDLGAPSIDESIDEWHITE
            return core.cdlgapsidesidewhiteDisplayShift(outputIdx);
         case 48: // CDLGRAVESTONEDOJI
            return core.cdlgravestonedojiDisplayShift(outputIdx);
         case 49: // CDLHAMMER
            return core.cdlhammerDisplayShift(outputIdx);
         case 50: // CDLHANGINGMAN
            return core.cdlhangingmanDisplayShift(outputIdx);
         case 51: // CDLHARAMI
            return core.cdlharamiDisplayShift(outputIdx);
         case 52: // CDLHARAMICROSS
            return core.cdlharamicrossDisplayShift(outputIdx);
         case 53: // CDLHIGHWAVE
            return core.cdlhighwaveDisplayShift(outputIdx);
         case 54: // CDLHIKKAKE
            return core.cdlhikkakeDisplayShift(outputIdx);
         case 55: // CDLHIKKAKEMOD
            return core.cdlhikkakemodDisplayShift(outputIdx);
         case 56: // CDLHOMINGPIGEON
            return core.cdlhomingpigeonDisplayShift(outputIdx);
         case 57: // CDLIDENTICAL3CROWS
            return core.cdlidentical3crowsDisplayShift(outputIdx);
         case 58: // CDLINNECK
            return core.cdlinneckDisplayShift(outputIdx);
         case 59: // CDLINVERTEDHAMMER
            return core.cdlinvertedhammerDisplayShift(outputIdx);
         case 60: // CDLKICKING
            return core.cdlkickingDisplayShift(outputIdx);
         case 61: // CDLKICKINGBYLENGTH
            return core.cdlkickingbylengthDisplayShift(outputIdx);
         case 62: // CDLLADDERBOTTOM
            return core.cdlladderbottomDisplayShift(outputIdx);
         case 63: // CDLLONGLEGGEDDOJI
            return core.cdllongleggeddojiDisplayShift(outputIdx);
         default:
            throw new TALibArgumentException("no such function: " + h.info().name(), RetCode.INVALID_HANDLE);
      }
   }

   private static int displayShift1(ParamHolder h, int outputIdx) {
      Core core = h.core();
      switch (h.route()) {
         case 64: // CDLLONGLINE
            return core.cdllonglineDisplayShift(outputIdx);
         case 65: // CDLMARUBOZU
            return core.cdlmarubozuDisplayShift(outputIdx);
         case 66: // CDLMATCHINGLOW
            return core.cdlmatchinglowDisplayShift(outputIdx);
         case 67: // CDLMATHOLD
            return core.cdlmatholdDisplayShift(h.realOpt(0), outputIdx);
         case 68: // CDLMORNINGDOJISTAR
            return core.cdlmorningdojistarDisplayShift(h.realOpt(0), outputIdx);
         case 69: // CDLMORNINGSTAR
            return core.cdlmorningstarDisplayShift(h.realOpt(0), outputIdx);
         case 70: // CDLONNECK
            return core.cdlonneckDisplayShift(outputIdx);
         case 71: // CDLPIERCING
            return core.cdlpiercingDisplayShift(outputIdx);
         case 72: // CDLRICKSHAWMAN
            return core.cdlrickshawmanDisplayShift(outputIdx);
         case 73: // CDLRISEFALL3METHODS
            return core.cdlrisefall3methodsDisplayShift(outputIdx);
         case 74: // CDLSEPARATINGLINES
            return core.cdlseparatinglinesDisplayShift(outputIdx);
         case 75: // CDLSHOOTINGSTAR
            return core.cdlshootingstarDisplayShift(outputIdx);
         case 76: // CDLSHORTLINE
            return core.cdlshortlineDisplayShift(outputIdx);
         case 77: // CDLSPINNINGTOP
            return core.cdlspinningtopDisplayShift(outputIdx);
         case 78: // CDLSTALLEDPATTERN
            return core.cdlstalledpatternDisplayShift(outputIdx);
         case 79: // CDLSTICKSANDWICH
            return core.cdlsticksandwichDisplayShift(outputIdx);
         case 80: // CDLTAKURI
            return core.cdltakuriDisplayShift(outputIdx);
         case 81: // CDLTASUKIGAP
            return core.cdltasukigapDisplayShift(outputIdx);
         case 82: // CDLTHRUSTING
            return core.cdlthrustingDisplayShift(outputIdx);
         case 83: // CDLTRISTAR
            return core.cdltristarDisplayShift(outputIdx);
         case 84: // CDLUNIQUE3RIVER
            return core.cdlunique3riverDisplayShift(outputIdx);
         case 85: // CDLUPSIDEGAP2CROWS
            return core.cdlupsidegap2crowsDisplayShift(outputIdx);
         case 86: // CDLXSIDEGAP3METHODS
            return core.cdlxsidegap3methodsDisplayShift(outputIdx);
         case 87: // CE
            return core.ceDisplayShift(h.intOpt(0), h.realOpt(1), outputIdx);
         case 88: // CEIL
            return core.ceilDisplayShift(outputIdx);
         case 89: // CG
            return core.cgDisplayShift(h.intOpt(0), outputIdx);
         case 90: // CHOP
            return core.chopDisplayShift(h.intOpt(0), outputIdx);
         case 91: // CHOPTR
            return core.choptrDisplayShift(h.intOpt(0), outputIdx);
         case 92: // CKSP
            return core.ckspDisplayShift(h.intOpt(0), h.realOpt(1), h.intOpt(2), outputIdx);
         case 93: // CMF
            return core.cmfDisplayShift(h.intOpt(0), outputIdx);
         case 94: // CMO
            return core.cmoDisplayShift(h.intOpt(0), outputIdx);
         case 95: // CMOU
            return core.cmouDisplayShift(h.intOpt(0), outputIdx);
         case 96: // COPPOCK
            return core.coppockDisplayShift(h.intOpt(0), h.intOpt(1), h.intOpt(2), outputIdx);
         case 97: // CORREL
            return core.correlDisplayShift(h.intOpt(0), outputIdx);
         case 98: // COS
            return core.cosDisplayShift(outputIdx);
         case 99: // COSH
            return core.coshDisplayShift(outputIdx);
         case 100: // CRSI
            return core.crsiDisplayShift(h.intOpt(0), h.intOpt(1), h.intOpt(2), outputIdx);
         case 101: // CTI
            return core.ctiDisplayShift(h.intOpt(0), outputIdx);
         case 102: // CUMSUM
            return core.cumsumDisplayShift(outputIdx);
         case 103: // CVI
            return core.cviDisplayShift(h.intOpt(0), h.intOpt(1), outputIdx);
         case 104: // DEMA
            return core.demaDisplayShift(h.intOpt(0), outputIdx);
         case 105: // DIV
            return core.divDisplayShift(outputIdx);
         case 106: // DONCHIAN
            return core.donchianDisplayShift(h.intOpt(0), outputIdx);
         case 107: // DOSC
            return core.doscDisplayShift(h.intOpt(0), h.intOpt(1), h.intOpt(2), h.intOpt(3), outputIdx);
         case 108: // DPO
            return core.dpoDisplayShift(h.intOpt(0), outputIdx);
         case 109: // DX
            return core.dxDisplayShift(h.intOpt(0), outputIdx);
         case 110: // EFI
            return core.efiDisplayShift(h.intOpt(0), outputIdx);
         case 111: // EMA
            return core.emaDisplayShift(h.intOpt(0), outputIdx);
         case 112: // EMV
            return core.emvDisplayShift(h.intOpt(0), h.realOpt(1), outputIdx);
         case 113: // ER
            return core.erDisplayShift(h.intOpt(0), outputIdx);
         case 114: // ERI
            return core.eriDisplayShift(h.intOpt(0), outputIdx);
         case 115: // EXP
            return core.expDisplayShift(outputIdx);
         case 116: // FISHER
            return core.fisherDisplayShift(h.intOpt(0), outputIdx);
         case 117: // FLOOR
            return core.floorDisplayShift(outputIdx);
         case 118: // FOSC
            return core.foscDisplayShift(h.intOpt(0), outputIdx);
         case 119: // FRACTAL
            return core.fractalDisplayShift(h.intOpt(0), h.intOpt(1), outputIdx);
         case 120: // FRAMA
            return core.framaDisplayShift(h.intOpt(0), outputIdx);
         case 121: // GATOR
            return core.gatorDisplayShift(h.intOpt(0), h.intOpt(1), h.intOpt(2), h.intOpt(3), h.intOpt(4), h.intOpt(5), outputIdx);
         case 122: // HA
            return core.haDisplayShift(outputIdx);
         case 123: // HMA
            return core.hmaDisplayShift(h.intOpt(0), outputIdx);
         case 124: // HT_DCPERIOD
            return core.htDcperiodDisplayShift(outputIdx);
         case 125: // HT_DCPHASE
            return core.htDcphaseDisplayShift(outputIdx);
         case 126: // HT_PHASOR
            return core.htPhasorDisplayShift(outputIdx);
         case 127: // HT_SINE
            return core.htSineDisplayShift(outputIdx);
         default:
            throw new TALibArgumentException("no such function: " + h.info().name(), RetCode.INVALID_HANDLE);
      }
   }

   private static int displayShift2(ParamHolder h, int outputIdx) {
      Core core = h.core();
      switch (h.route()) {
         case 128: // HT_TRENDLINE
            return core.htTrendlineDisplayShift(outputIdx);
         case 129: // HT_TRENDMODE
            return core.htTrendmodeDisplayShift(outputIdx);
         case 130: // IBS
            return core.ibsDisplayShift(outputIdx);
         case 131: // ICHIMOKU
            return core.ichimokuDisplayShift(h.intOpt(0), h.intOpt(1), h.intOpt(2), outputIdx);
         case 132: // IMI
            return core.imiDisplayShift(h.intOpt(0), outputIdx);
         case 133: // KAMA
            return core.kamaDisplayShift(h.intOpt(0), outputIdx);
         case 134: // KC
            return core.kcDisplayShift(h.intOpt(0), h.intOpt(1), h.realOpt(2), outputIdx);
         case 135: // KDJ
            return core.kdjDisplayShift(h.intOpt(0), h.intOpt(1), h.maTypeOpt(2), h.intOpt(3), h.maTypeOpt(4), outputIdx);
         case 136: // KST
            return core.kstDisplayShift(h.intOpt(0), h.intOpt(1), h.intOpt(2), h.intOpt(3), h.intOpt(4), h.intOpt(5), h.intOpt(6), h.intOpt(7), h.intOpt(8), outputIdx);
         case 137: // KSTEXT
            return core.kstextDisplayShift(h.intOpt(0), h.intOpt(1), h.intOpt(2), h.intOpt(3), h.intOpt(4), h.intOpt(5), h.intOpt(6), h.intOpt(7), h.intOpt(8), h.maTypeOpt(9), h.maTypeOpt(10), outputIdx);
         case 138: // KURTOSIS
            return core.kurtosisDisplayShift(h.intOpt(0), outputIdx);
         case 139: // KVO
            return core.kvoDisplayShift(h.intOpt(0), h.intOpt(1), h.intOpt(2), outputIdx);
         case 140: // LINEARREG
            return core.linearregDisplayShift(h.intOpt(0), outputIdx);
         case 141: // LINEARREG_ANGLE
            return core.linearregAngleDisplayShift(h.intOpt(0), outputIdx);
         case 142: // LINEARREG_INTERCEPT
            return core.linearregInterceptDisplayShift(h.intOpt(0), outputIdx);
         case 143: // LINEARREG_SLOPE
            return core.linearregSlopeDisplayShift(h.intOpt(0), outputIdx);
         case 144: // LN
            return core.lnDisplayShift(outputIdx);
         case 145: // LOG10
            return core.log10DisplayShift(outputIdx);
         case 146: // MA
            return core.maDisplayShift(h.intOpt(0), h.maTypeOpt(1), outputIdx);
         case 147: // MACD
            return core.macdDisplayShift(h.intOpt(0), h.intOpt(1), h.intOpt(2), outputIdx);
         case 148: // MACDEXT
            return core.macdextDisplayShift(h.intOpt(0), h.maTypeOpt(1), h.intOpt(2), h.maTypeOpt(3), h.intOpt(4), h.maTypeOpt(5), outputIdx);
         case 149: // MACDFIX
            return core.macdfixDisplayShift(h.intOpt(0), outputIdx);
         case 150: // MACDV
            return core.macdvDisplayShift(h.intOpt(0), h.intOpt(1), h.intOpt(2), outputIdx);
         case 151: // MAENV
            return core.maenvDisplayShift(h.intOpt(0), h.realOpt(1), h.realOpt(2), h.maTypeOpt(3), outputIdx);
         case 152: // MAMA
            return core.mamaDisplayShift(h.realOpt(0), h.realOpt(1), outputIdx);
         case 153: // MARKETFI
            return core.marketfiDisplayShift(outputIdx);
         case 154: // MASSI
            return core.massiDisplayShift(h.intOpt(0), h.intOpt(1), outputIdx);
         case 155: // MAVP
            return core.mavpDisplayShift(h.intOpt(0), h.intOpt(1), h.maTypeOpt(2), outputIdx);
         case 156: // MAX
            return core.maxDisplayShift(h.intOpt(0), outputIdx);
         case 157: // MAXINDEX
            return core.maxindexDisplayShift(h.intOpt(0), outputIdx);
         case 158: // MCGD
            return core.mcgdDisplayShift(h.intOpt(0), outputIdx);
         case 159: // MEDIAN
            return core.medianDisplayShift(h.intOpt(0), outputIdx);
         case 160: // MEDPRICE
            return core.medpriceDisplayShift(outputIdx);
         case 161: // MFI
            return core.mfiDisplayShift(h.intOpt(0), outputIdx);
         case 162: // MIDPOINT
            return core.midpointDisplayShift(h.intOpt(0), outputIdx);
         case 163: // MIDPRICE
            return core.midpriceDisplayShift(h.intOpt(0), outputIdx);
         case 164: // MIN
            return core.minDisplayShift(h.intOpt(0), outputIdx);
         case 165: // MININDEX
            return core.minindexDisplayShift(h.intOpt(0), outputIdx);
         case 166: // MINMAX
            return core.minmaxDisplayShift(h.intOpt(0), outputIdx);
         case 167: // MINMAXINDEX
            return core.minmaxindexDisplayShift(h.intOpt(0), outputIdx);
         case 168: // MINUS_DI
            return core.minusDiDisplayShift(h.intOpt(0), outputIdx);
         case 169: // MINUS_DM
            return core.minusDmDisplayShift(h.intOpt(0), outputIdx);
         case 170: // MOM
            return core.momDisplayShift(h.intOpt(0), outputIdx);
         case 171: // MULT
            return core.multDisplayShift(outputIdx);
         case 172: // NATR
            return core.natrDisplayShift(h.intOpt(0), outputIdx);
         case 173: // NVI
            return core.nviDisplayShift(outputIdx);
         case 174: // OBV
            return core.obvDisplayShift(outputIdx);
         case 175: // PERCENTB
            return core.percentbDisplayShift(h.intOpt(0), h.realOpt(1), h.realOpt(2), h.maTypeOpt(3), outputIdx);
         case 176: // PERCENTILE
            return core.percentileDisplayShift(h.intOpt(0), h.realOpt(1), outputIdx);
         case 177: // PERCENTRANK
            return core.percentrankDisplayShift(h.intOpt(0), outputIdx);
         case 178: // PLUS_DI
            return core.plusDiDisplayShift(h.intOpt(0), outputIdx);
         case 179: // PLUS_DM
            return core.plusDmDisplayShift(h.intOpt(0), outputIdx);
         case 180: // PPO
            return core.ppoDisplayShift(h.intOpt(0), h.intOpt(1), h.maTypeOpt(2), outputIdx);
         case 181: // PSO
            return core.psoDisplayShift(h.intOpt(0), h.intOpt(1), outputIdx);
         case 182: // PVI
            return core.pviDisplayShift(outputIdx);
         case 183: // PVO
            return core.pvoDisplayShift(h.intOpt(0), h.intOpt(1), h.maTypeOpt(2), outputIdx);
         case 184: // PVT
            return core.pvtDisplayShift(outputIdx);
         case 185: // QSTICK
            return core.qstickDisplayShift(h.intOpt(0), outputIdx);
         case 186: // RMA
            return core.rmaDisplayShift(h.intOpt(0), outputIdx);
         case 187: // ROC
            return core.rocDisplayShift(h.intOpt(0), outputIdx);
         case 188: // ROCP
            return core.rocpDisplayShift(h.intOpt(0), outputIdx);
         case 189: // ROCR
            return core.rocrDisplayShift(h.intOpt(0), outputIdx);
         case 190: // ROCR100
            return core.rocr100DisplayShift(h.intOpt(0), outputIdx);
         case 191: // ROGERSSATCHELL
            return core.rogerssatchellDisplayShift(h.intOpt(0), h.realOpt(1), outputIdx);
         default:
            throw new TALibArgumentException("no such function: " + h.info().name(), RetCode.INVALID_HANDLE);
      }
   }

   private static int displayShift3(ParamHolder h, int outputIdx) {
      Core core = h.core();
      switch (h.route()) {
         case 192: // RSI
            return core.rsiDisplayShift(h.intOpt(0), outputIdx);
         case 193: // RVI
            return core.rviDisplayShift(h.intOpt(0), h.intOpt(1), outputIdx);
         case 194: // RVIR
            return core.rvirDisplayShift(h.intOpt(0), h.intOpt(1), outputIdx);
         case 195: // RVOL
            return core.rvolDisplayShift(h.intOpt(0), outputIdx);
         case 196: // SAR
            return core.sarDisplayShift(h.realOpt(0), h.realOpt(1), outputIdx);
         case 197: // SAREXT
            return core.sarextDisplayShift(h.realOpt(0), h.realOpt(1), h.realOpt(2), h.realOpt(3), h.realOpt(4), h.realOpt(5), h.realOpt(6), h.realOpt(7), outputIdx);
         case 198: // SHARPE
            return core.sharpeDisplayShift(h.intOpt(0), h.realOpt(1), h.realOpt(2), outputIdx);
         case 199: // SI
            return core.siDisplayShift(h.realOpt(0), outputIdx);
         case 200: // SIN
            return core.sinDisplayShift(outputIdx);
         case 201: // SINH
            return core.sinhDisplayShift(outputIdx);
         case 202: // SKEW
            return core.skewDisplayShift(h.intOpt(0), outputIdx);
         case 203: // SMA
            return core.smaDisplayShift(h.intOpt(0), outputIdx);
         case 204: // SMI
            return core.smiDisplayShift(h.intOpt(0), h.intOpt(1), h.intOpt(2), h.intOpt(3), outputIdx);
         case 205: // SORTINO
            return core.sortinoDisplayShift(h.intOpt(0), h.realOpt(1), h.realOpt(2), outputIdx);
         case 206: // SQRT
            return core.sqrtDisplayShift(outputIdx);
         case 207: // SQZMOM
            return core.sqzmomDisplayShift(h.intOpt(0), h.realOpt(1), h.intOpt(2), h.realOpt(3), h.realOpt(4), h.realOpt(5), outputIdx);
         case 208: // SSF
            return core.ssfDisplayShift(h.intOpt(0), outputIdx);
         case 209: // STC
            return core.stcDisplayShift(h.intOpt(0), h.intOpt(1), h.intOpt(2), outputIdx);
         case 210: // STDDEV
            return core.stddevDisplayShift(h.intOpt(0), h.realOpt(1), outputIdx);
         case 211: // STOCH
            return core.stochDisplayShift(h.intOpt(0), h.intOpt(1), h.maTypeOpt(2), h.intOpt(3), h.maTypeOpt(4), outputIdx);
         case 212: // STOCHF
            return core.stochfDisplayShift(h.intOpt(0), h.intOpt(1), h.maTypeOpt(2), outputIdx);
         case 213: // STOCHRSI
            return core.stochrsiDisplayShift(h.intOpt(0), h.intOpt(1), h.intOpt(2), h.maTypeOpt(3), outputIdx);
         case 214: // SUB
            return core.subDisplayShift(outputIdx);
         case 215: // SUM
            return core.sumDisplayShift(h.intOpt(0), outputIdx);
         case 216: // SUPERTREND
            return core.supertrendDisplayShift(h.intOpt(0), h.realOpt(1), outputIdx);
         case 217: // SWAK_2PHP
            return core.swak2phpDisplayShift(h.intOpt(0), outputIdx);
         case 218: // SWAK_BP
            return core.swakBpDisplayShift(h.intOpt(0), h.realOpt(1), outputIdx);
         case 219: // SWAK_BUTTER
            return core.swakButterDisplayShift(h.intOpt(0), outputIdx);
         case 220: // SWAK_GAUSS
            return core.swakGaussDisplayShift(h.intOpt(0), outputIdx);
         case 221: // SWAK_HP
            return core.swakHpDisplayShift(h.intOpt(0), outputIdx);
         case 222: // T3
            return core.t3DisplayShift(h.intOpt(0), h.realOpt(1), outputIdx);
         case 223: // TAN
            return core.tanDisplayShift(outputIdx);
         case 224: // TANH
            return core.tanhDisplayShift(outputIdx);
         case 225: // TEMA
            return core.temaDisplayShift(h.intOpt(0), outputIdx);
         case 226: // TRANGE
            return core.trangeDisplayShift(outputIdx);
         case 227: // TRIMA
            return core.trimaDisplayShift(h.intOpt(0), outputIdx);
         case 228: // TRIX
            return core.trixDisplayShift(h.intOpt(0), outputIdx);
         case 229: // TSF
            return core.tsfDisplayShift(h.intOpt(0), outputIdx);
         case 230: // TSI
            return core.tsiDisplayShift(h.intOpt(0), h.intOpt(1), outputIdx);
         case 231: // TYPPRICE
            return core.typpriceDisplayShift(outputIdx);
         case 232: // ULTOSC
            return core.ultoscDisplayShift(h.intOpt(0), h.intOpt(1), h.intOpt(2), outputIdx);
         case 233: // VAR
            return core.varDisplayShift(h.intOpt(0), h.realOpt(1), outputIdx);
         case 234: // VHF
            return core.vhfDisplayShift(h.intOpt(0), outputIdx);
         case 235: // VIDYA
            return core.vidyaDisplayShift(h.intOpt(0), h.intOpt(1), outputIdx);
         case 236: // VORTEX
            return core.vortexDisplayShift(h.intOpt(0), outputIdx);
         case 237: // VWAP
            return core.vwapDisplayShift(outputIdx);
         case 238: // VWMA
            return core.vwmaDisplayShift(h.intOpt(0), outputIdx);
         case 239: // WAD
            return core.wadDisplayShift(outputIdx);
         case 240: // WAVETREND
            return core.wavetrendDisplayShift(h.intOpt(0), h.intOpt(1), h.intOpt(2), outputIdx);
         case 241: // WCLPRICE
            return core.wclpriceDisplayShift(outputIdx);
         case 242: // WILLR
            return core.willrDisplayShift(h.intOpt(0), outputIdx);
         case 243: // WMA
            return core.wmaDisplayShift(h.intOpt(0), outputIdx);
         case 244: // ZIGZAG
            return core.zigzagDisplayShift(h.realOpt(0), h.intOpt(1), outputIdx);
         case 245: // ZLEMA
            return core.zlemaDisplayShift(h.intOpt(0), outputIdx);
         default:
            throw new TALibArgumentException("no such function: " + h.info().name(), RetCode.INVALID_HANDLE);
      }
   }
}
