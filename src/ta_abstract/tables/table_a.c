/* TA-LIB Copyright (c) 1999-2025, Mario Fortier
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

/*********************************************************************
 * This file contains only TA functions starting with the letter 'A' *
 *********************************************************************/
#include <stddef.h>
#include "ta_abstract.h"
#include "ta_def_ui.h"

/* ACCBANDS BEGIN */
const TA_OutputParameterInfo TA_DEF_UI_Output_Real_ACCBANDS_UpperBand =
                               { TA_Output_Real, "outRealUpperBand", TA_OUT_UPPER_LIMIT };

const TA_OutputParameterInfo TA_DEF_UI_Output_Real_ACCBANDS_MiddleBand =
                               { TA_Output_Real, "outRealMiddleBand", TA_OUT_LINE };

const TA_OutputParameterInfo TA_DEF_UI_Output_Real_ACCBANDS_LowerBand =
                               { TA_Output_Real, "outRealLowerBand", TA_OUT_LOWER_LIMIT };

static const TA_InputParameterInfo    *TA_ACCBANDS_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_HLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_ACCBANDS_Outputs[]   =
{
  &TA_DEF_UI_Output_Real_ACCBANDS_UpperBand,
  &TA_DEF_UI_Output_Real_ACCBANDS_MiddleBand,
  &TA_DEF_UI_Output_Real_ACCBANDS_LowerBand,
  NULL
};

static const TA_OptInputParameterInfo *TA_ACCBANDS_OptInputs[] =
{ &TA_DEF_UI_TimePeriod_20_MINIMUM2,
  NULL
};

DEF_FUNCTION( ACCBANDS,
              TA_GroupId_OverlapStudies,
              "Acceleration Bands",
              "Accbands",
              TA_FUNC_FLG_OVERLAP
             );
/* ACCBANDS END */

/* ACOS BEGIN */
static const TA_InputParameterInfo    *TA_ACOS_Inputs[]    =
{
  &TA_DEF_UI_Input_Real,
  NULL
};

static const TA_OutputParameterInfo   *TA_ACOS_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_ACOS_OptInputs[] =
{ NULL };

DEF_FUNCTION( ACOS,
              TA_GroupId_MathTransform,
              "Vector Trigonometric ACos",
              "Acos",
              TA_FUNC_FLG_STREAM
             );
/* ACOS END */

/* AD BEGIN */
static const TA_InputParameterInfo    *TA_AD_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_HLCV,
  NULL
};

static const TA_OutputParameterInfo   *TA_AD_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_AD_OptInputs[] =
{ NULL };

DEF_FUNCTION( AD,
              TA_GroupId_VolumeIndicators,
              "Chaikin A/D Line",
              "Ad",
              TA_FUNC_FLG_STREAM
             );
/* AD END */

/* ADD BEGIN */
static const TA_InputParameterInfo    *TA_ADD_Inputs[]    =
{
  &TA_DEF_UI_Input_Real0,
  &TA_DEF_UI_Input_Real1,
  NULL
};

static const TA_OutputParameterInfo   *TA_ADD_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_ADD_OptInputs[] =
{ NULL };

DEF_FUNCTION( ADD,
              TA_GroupId_MathOperators,
              "Vector Arithmetic Add",
              "Add",
              TA_FUNC_FLG_STREAM
             );
/* ADD END */

/* ADOSC BEGIN */
static const TA_OptInputParameterInfo TA_DEF_UI_D_ADOSC_FastPeriod =
{
   TA_OptInput_IntegerRange,
   "optInFastPeriod",
   0,

   "Fast Period",
   (const void *)&TA_DEF_TimePeriod_Positive_Minimum2,
   3,
   "Number of period for the fast MA",

   NULL
};

static const TA_OptInputParameterInfo TA_DEF_UI_D_ADOSC_SlowPeriod =
{
   TA_OptInput_IntegerRange,
   "optInSlowPeriod",
   0,

   "Slow Period",
   (const void *)&TA_DEF_TimePeriod_Positive_Minimum2,
   10,
   "Number of period for the slow MA",

   NULL
};

static const TA_InputParameterInfo    *TA_ADOSC_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_HLCV,
  NULL
};

static const TA_OutputParameterInfo   *TA_ADOSC_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_ADOSC_OptInputs[] =
{ &TA_DEF_UI_D_ADOSC_FastPeriod,
  &TA_DEF_UI_D_ADOSC_SlowPeriod,
  NULL
};

DEF_FUNCTION( ADOSC,
              TA_GroupId_VolumeIndicators,
              "Chaikin A/D Oscillator",
              "AdOsc",
              TA_FUNC_FLG_STREAM
             );
/* ADOSC END */

/* ADX BEGIN */
static const TA_InputParameterInfo    *TA_ADX_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_HLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_ADX_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_ADX_OptInputs[] =
{ &TA_DEF_UI_TimePeriod_14_MINIMUM2,
  NULL
};

DEF_FUNCTION( ADX,
              TA_GroupId_MomentumIndicators,
              "Average Directional Movement Index",
              "Adx",
              TA_FUNC_FLG_UNST_PER | TA_FUNC_FLG_STREAM
             );
/* ADX END */

/* ADXR BEGIN */
static const TA_InputParameterInfo    *TA_ADXR_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_HLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_ADXR_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_ADXR_OptInputs[] =
{ &TA_DEF_UI_TimePeriod_14_MINIMUM2,
  NULL
};

DEF_FUNCTION( ADXR,
              TA_GroupId_MomentumIndicators,
              "Average Directional Movement Index Rating",
              "Adxr",
              TA_FUNC_FLG_UNST_PER
             );
/* ADXR END */

/* APO BEGIN */
static const TA_InputParameterInfo    *TA_APO_Inputs[]    =
{
  &TA_DEF_UI_Input_Real,
  NULL
};

static const TA_OutputParameterInfo   *TA_APO_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_APO_OptInputs[] =
{ &TA_DEF_UI_Fast_Period,
  &TA_DEF_UI_Slow_Period,
  &TA_DEF_UI_MA_Method,
  NULL
};

DEF_FUNCTION( APO,
              TA_GroupId_MomentumIndicators,
              "Absolute Price Oscillator",
              "Apo",
              0
             );
/* APO END */

/* AROON BEGIN */
const TA_OutputParameterInfo TA_DEF_UI_Output_Real_AROON_outAroonDown =
                               { TA_Output_Real, "outAroonDown", TA_OUT_DASH_LINE };

const TA_OutputParameterInfo TA_DEF_UI_Output_Real_AROON_outAroonUp =
                               { TA_Output_Real, "outAroonUp", TA_OUT_LINE };

static const TA_InputParameterInfo    *TA_AROON_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_HL,
  NULL
};

static const TA_OutputParameterInfo   *TA_AROON_Outputs[]   =
{
  &TA_DEF_UI_Output_Real_AROON_outAroonDown,
  &TA_DEF_UI_Output_Real_AROON_outAroonUp,
  NULL
};

static const TA_OptInputParameterInfo *TA_AROON_OptInputs[] =
{ &TA_DEF_UI_TimePeriod_14_MINIMUM2,
  NULL
};

DEF_FUNCTION( AROON,
              TA_GroupId_MomentumIndicators,
              "Aroon",
              "Aroon",
              TA_FUNC_FLG_STREAM
             );
/* AROON END */

/* AROONOSC BEGIN */
static const TA_InputParameterInfo    *TA_AROONOSC_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_HL,
  NULL
};

static const TA_OutputParameterInfo   *TA_AROONOSC_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_AROONOSC_OptInputs[] =
{ &TA_DEF_UI_TimePeriod_14_MINIMUM2,
  NULL
};

DEF_FUNCTION( AROONOSC,
              TA_GroupId_MomentumIndicators,
              "Aroon Oscillator",
              "AroonOsc",
              TA_FUNC_FLG_STREAM
             );
/* AROONOSC END */

/* ASIN BEGIN */
static const TA_InputParameterInfo    *TA_ASIN_Inputs[]    =
{
  &TA_DEF_UI_Input_Real,
  NULL
};

static const TA_OutputParameterInfo   *TA_ASIN_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_ASIN_OptInputs[] =
{ NULL };

DEF_FUNCTION( ASIN,
              TA_GroupId_MathTransform,
              "Vector Trigonometric ASin",
              "Asin",
              TA_FUNC_FLG_STREAM
             );
/* ASIN END */

/* ATAN BEGIN */
static const TA_InputParameterInfo    *TA_ATAN_Inputs[]    =
{
  &TA_DEF_UI_Input_Real,
  NULL
};

static const TA_OutputParameterInfo   *TA_ATAN_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_ATAN_OptInputs[] =
{ NULL };

DEF_FUNCTION( ATAN,
              TA_GroupId_MathTransform,
              "Vector Trigonometric ATan",
              "Atan",
              TA_FUNC_FLG_STREAM
             );
/* ATAN END */

/* ATR BEGIN */
static const TA_InputParameterInfo    *TA_ATR_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_HLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_ATR_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_ATR_OptInputs[] =
{ &TA_DEF_UI_TimePeriod_14,
  NULL
};

DEF_FUNCTION( ATR,
              TA_GroupId_VolatilityIndicators,
              "Average True Range",
              "Atr",
              TA_FUNC_FLG_UNST_PER
             );
/* ATR END */

/* AVGDEV BEGIN */
static const TA_InputParameterInfo    *TA_AVGDEV_Inputs[]    =
{
  &TA_DEF_UI_Input_Real,
  NULL
};

static const TA_OutputParameterInfo   *TA_AVGDEV_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_AVGDEV_OptInputs[] =
{ &TA_DEF_UI_TimePeriod_14_MINIMUM2,
  NULL
};

DEF_FUNCTION( AVGDEV,
              TA_GroupId_PriceTransform,
              "Average Deviation",
              "AvgDev",
              TA_FUNC_FLG_OVERLAP
             );
/* AVGDEV END */

/* AVGPRICE BEGIN */
static const TA_InputParameterInfo    *TA_AVGPRICE_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_OHLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_AVGPRICE_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_AVGPRICE_OptInputs[] =
{ NULL };

DEF_FUNCTION( AVGPRICE,
              TA_GroupId_PriceTransform,
              "Average Price",
              "AvgPrice",
              TA_FUNC_FLG_OVERLAP | TA_FUNC_FLG_STREAM
             );
/* AVGPRICE END */

/****************************************************************************
 * Step 2 - Add your TA function to the table.
 *          Keep in alphabetical order. Must be NULL terminated.
 ****************************************************************************/
const TA_FuncDef *TA_DEF_TableA[] =
{
   ADD_TO_TABLE(ACCBANDS),
   ADD_TO_TABLE(ACOS),
   ADD_TO_TABLE(AD),
   ADD_TO_TABLE(ADD),
   ADD_TO_TABLE(ADOSC),
   ADD_TO_TABLE(ADX),
   ADD_TO_TABLE(ADXR),
   ADD_TO_TABLE(APO),
   ADD_TO_TABLE(AROON),
   ADD_TO_TABLE(AROONOSC),
   ADD_TO_TABLE(ASIN),
   ADD_TO_TABLE(ATAN),
   ADD_TO_TABLE(ATR),
   ADD_TO_TABLE(AVGDEV),
   ADD_TO_TABLE(AVGPRICE),
   NULL
};


/* Do not modify the following line. */
const unsigned int TA_DEF_TableASize =
              ((sizeof(TA_DEF_TableA)/sizeof(TA_FuncDef *))-1);

