/* TA-LIB Copyright (c) 1999-2004, Mario Fortier
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

/* Important: This file is automatically generated by gen_code.
 *            Any modification will be lost on next execution
 *            of gen_code.
 *
 * The goal of this file is to build the following global
 * constant:
 *       TA_PerGroupFuncDef
 *       TA_PerGroupSize
 *       TA_TotalNbFunction
 *
 * These constant allows mainly to speed optimize functionality related to
 * sequential access to TA_DefFunc by TA_GroupId (see ta_abstract.c)
 */
#include <stddef.h>
#include "ta_def_ui.h"
#include "ta_abstract.h"

extern const TA_FuncDef TA_DEF_MAX;
extern const TA_FuncDef TA_DEF_MIN;
extern const TA_FuncDef TA_DEF_BBANDS;
extern const TA_FuncDef TA_DEF_DEMA;
extern const TA_FuncDef TA_DEF_EMA;
extern const TA_FuncDef TA_DEF_HT_TRENDLINE;
extern const TA_FuncDef TA_DEF_KAMA;
extern const TA_FuncDef TA_DEF_MA;
extern const TA_FuncDef TA_DEF_MAMA;
extern const TA_FuncDef TA_DEF_MIDPRICE;
extern const TA_FuncDef TA_DEF_MIDPOINT;
extern const TA_FuncDef TA_DEF_SAR;
extern const TA_FuncDef TA_DEF_SAREXT;
extern const TA_FuncDef TA_DEF_SMA;
extern const TA_FuncDef TA_DEF_T3;
extern const TA_FuncDef TA_DEF_TEMA;
extern const TA_FuncDef TA_DEF_TRIMA;
extern const TA_FuncDef TA_DEF_WMA;
extern const TA_FuncDef TA_DEF_ATR;
extern const TA_FuncDef TA_DEF_TRANGE;
extern const TA_FuncDef TA_DEF_ADX;
extern const TA_FuncDef TA_DEF_ADXR;
extern const TA_FuncDef TA_DEF_APO;
extern const TA_FuncDef TA_DEF_AROON;
extern const TA_FuncDef TA_DEF_AROONOSC;
extern const TA_FuncDef TA_DEF_CCI;
extern const TA_FuncDef TA_DEF_DX;
extern const TA_FuncDef TA_DEF_MACD;
extern const TA_FuncDef TA_DEF_MACDEXT;
extern const TA_FuncDef TA_DEF_MACDFIX;
extern const TA_FuncDef TA_DEF_MFI;
extern const TA_FuncDef TA_DEF_MINUS_DI;
extern const TA_FuncDef TA_DEF_MINUS_DM;
extern const TA_FuncDef TA_DEF_MOM;
extern const TA_FuncDef TA_DEF_PPO;
extern const TA_FuncDef TA_DEF_PLUS_DI;
extern const TA_FuncDef TA_DEF_PLUS_DM;
extern const TA_FuncDef TA_DEF_ROC;
extern const TA_FuncDef TA_DEF_ROCP;
extern const TA_FuncDef TA_DEF_ROCR;
extern const TA_FuncDef TA_DEF_ROCR100;
extern const TA_FuncDef TA_DEF_RSI;
extern const TA_FuncDef TA_DEF_STOCH;
extern const TA_FuncDef TA_DEF_STOCHF;
extern const TA_FuncDef TA_DEF_STOCHRSI;
extern const TA_FuncDef TA_DEF_TRIX;
extern const TA_FuncDef TA_DEF_WILLR;
extern const TA_FuncDef TA_DEF_HT_DCPERIOD;
extern const TA_FuncDef TA_DEF_HT_DCPHASE;
extern const TA_FuncDef TA_DEF_HT_PHASOR;
extern const TA_FuncDef TA_DEF_HT_SINE;
extern const TA_FuncDef TA_DEF_HT_TRENDMODE;
extern const TA_FuncDef TA_DEF_AD;
extern const TA_FuncDef TA_DEF_ADOSC;
extern const TA_FuncDef TA_DEF_OBV;
extern const TA_FuncDef TA_DEF_CDLHIGHWAVE;
extern const TA_FuncDef TA_DEF_CDLLONGLINE;
extern const TA_FuncDef TA_DEF_CDLSHORTLINE;
extern const TA_FuncDef TA_DEF_CDLSPINNINGTOP;
extern const TA_FuncDef TA_DEF_CORREL;
extern const TA_FuncDef TA_DEF_LINEARREG;
extern const TA_FuncDef TA_DEF_LINEARREG_SLOPE;
extern const TA_FuncDef TA_DEF_LINEARREG_ANGLE;
extern const TA_FuncDef TA_DEF_LINEARREG_INTERCEPT;
extern const TA_FuncDef TA_DEF_STDDEV;
extern const TA_FuncDef TA_DEF_TSF;
extern const TA_FuncDef TA_DEF_VAR;
extern const TA_FuncDef TA_DEF_AVGPRICE;
extern const TA_FuncDef TA_DEF_MEDPRICE;
extern const TA_FuncDef TA_DEF_TYPPRICE;
extern const TA_FuncDef TA_DEF_WCLPRICE;

const TA_FuncDef *TA_PerGroupFunc_0[] = {
&TA_DEF_MAX,
&TA_DEF_MIN,
NULL };
#define SIZE_GROUP_0 ((sizeof(TA_PerGroupFunc_0)/sizeof(const TA_FuncDef *))-1)

const TA_FuncDef *TA_PerGroupFunc_1[] = {
NULL };
#define SIZE_GROUP_1 ((sizeof(TA_PerGroupFunc_1)/sizeof(const TA_FuncDef *))-1)

const TA_FuncDef *TA_PerGroupFunc_2[] = {
&TA_DEF_BBANDS,
&TA_DEF_DEMA,
&TA_DEF_EMA,
&TA_DEF_HT_TRENDLINE,
&TA_DEF_KAMA,
&TA_DEF_MA,
&TA_DEF_MAMA,
&TA_DEF_MIDPRICE,
&TA_DEF_MIDPOINT,
&TA_DEF_SAR,
&TA_DEF_SAREXT,
&TA_DEF_SMA,
&TA_DEF_T3,
&TA_DEF_TEMA,
&TA_DEF_TRIMA,
&TA_DEF_WMA,
NULL };
#define SIZE_GROUP_2 ((sizeof(TA_PerGroupFunc_2)/sizeof(const TA_FuncDef *))-1)

const TA_FuncDef *TA_PerGroupFunc_3[] = {
&TA_DEF_ATR,
&TA_DEF_TRANGE,
NULL };
#define SIZE_GROUP_3 ((sizeof(TA_PerGroupFunc_3)/sizeof(const TA_FuncDef *))-1)

const TA_FuncDef *TA_PerGroupFunc_4[] = {
&TA_DEF_ADX,
&TA_DEF_ADXR,
&TA_DEF_APO,
&TA_DEF_AROON,
&TA_DEF_AROONOSC,
&TA_DEF_CCI,
&TA_DEF_DX,
&TA_DEF_MACD,
&TA_DEF_MACDEXT,
&TA_DEF_MACDFIX,
&TA_DEF_MFI,
&TA_DEF_MINUS_DI,
&TA_DEF_MINUS_DM,
&TA_DEF_MOM,
&TA_DEF_PPO,
&TA_DEF_PLUS_DI,
&TA_DEF_PLUS_DM,
&TA_DEF_ROC,
&TA_DEF_ROCP,
&TA_DEF_ROCR,
&TA_DEF_ROCR100,
&TA_DEF_RSI,
&TA_DEF_STOCH,
&TA_DEF_STOCHF,
&TA_DEF_STOCHRSI,
&TA_DEF_TRIX,
&TA_DEF_WILLR,
NULL };
#define SIZE_GROUP_4 ((sizeof(TA_PerGroupFunc_4)/sizeof(const TA_FuncDef *))-1)

const TA_FuncDef *TA_PerGroupFunc_5[] = {
&TA_DEF_HT_DCPERIOD,
&TA_DEF_HT_DCPHASE,
&TA_DEF_HT_PHASOR,
&TA_DEF_HT_SINE,
&TA_DEF_HT_TRENDMODE,
NULL };
#define SIZE_GROUP_5 ((sizeof(TA_PerGroupFunc_5)/sizeof(const TA_FuncDef *))-1)

const TA_FuncDef *TA_PerGroupFunc_6[] = {
&TA_DEF_AD,
&TA_DEF_ADOSC,
&TA_DEF_OBV,
NULL };
#define SIZE_GROUP_6 ((sizeof(TA_PerGroupFunc_6)/sizeof(const TA_FuncDef *))-1)

const TA_FuncDef *TA_PerGroupFunc_7[] = {
&TA_DEF_CDLHIGHWAVE,
&TA_DEF_CDLLONGLINE,
&TA_DEF_CDLSHORTLINE,
&TA_DEF_CDLSPINNINGTOP,
NULL };
#define SIZE_GROUP_7 ((sizeof(TA_PerGroupFunc_7)/sizeof(const TA_FuncDef *))-1)

const TA_FuncDef *TA_PerGroupFunc_8[] = {
&TA_DEF_CORREL,
&TA_DEF_LINEARREG,
&TA_DEF_LINEARREG_SLOPE,
&TA_DEF_LINEARREG_ANGLE,
&TA_DEF_LINEARREG_INTERCEPT,
&TA_DEF_STDDEV,
&TA_DEF_TSF,
&TA_DEF_VAR,
NULL };
#define SIZE_GROUP_8 ((sizeof(TA_PerGroupFunc_8)/sizeof(const TA_FuncDef *))-1)

const TA_FuncDef *TA_PerGroupFunc_9[] = {
&TA_DEF_AVGPRICE,
&TA_DEF_MEDPRICE,
&TA_DEF_TYPPRICE,
&TA_DEF_WCLPRICE,
NULL };
#define SIZE_GROUP_9 ((sizeof(TA_PerGroupFunc_9)/sizeof(const TA_FuncDef *))-1)
/* Generated */ const TA_FuncDef **TA_PerGroupFuncDef[10] = {
&TA_PerGroupFunc_0[0],
&TA_PerGroupFunc_1[0],
&TA_PerGroupFunc_2[0],
&TA_PerGroupFunc_3[0],
&TA_PerGroupFunc_4[0],
&TA_PerGroupFunc_5[0],
&TA_PerGroupFunc_6[0],
&TA_PerGroupFunc_7[0],
&TA_PerGroupFunc_8[0],
&TA_PerGroupFunc_9[0]
/* Generated */ };

/* Generated */ const unsigned int TA_PerGroupSize[10] = {
SIZE_GROUP_0,
SIZE_GROUP_1,
SIZE_GROUP_2,
SIZE_GROUP_3,
SIZE_GROUP_4,
SIZE_GROUP_5,
SIZE_GROUP_6,
SIZE_GROUP_7,
SIZE_GROUP_8,
SIZE_GROUP_9
/* Generated */ };

/* Generated */ const unsigned int TA_TotalNbFunction =
SIZE_GROUP_0+
SIZE_GROUP_1+
SIZE_GROUP_2+
SIZE_GROUP_3+
SIZE_GROUP_4+
SIZE_GROUP_5+
SIZE_GROUP_6+
SIZE_GROUP_7+
SIZE_GROUP_8+
SIZE_GROUP_9;

/***************/
/* End of File */
/***************/

