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
 * This file contains only TA functions starting with the letter 'M' *
 *********************************************************************/
#include <stddef.h>
#include "ta_abstract.h"
#include "ta_def_ui.h"

/* MA BEGIN */
static const TA_InputParameterInfo    *TA_MA_Inputs[]    =
{
  &TA_DEF_UI_Input_Real,
  NULL
};

static const TA_OutputParameterInfo   *TA_MA_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_MA_OptInputs[] =
{ &TA_DEF_UI_TimePeriod_30,
  &TA_DEF_UI_MA_Method,
  NULL
};

DEF_FUNCTION( MA,
              TA_GroupId_OverlapStudies,
              "Moving average",
              "MovingAverage",
              TA_FUNC_FLG_OVERLAP | TA_FUNC_FLG_STREAM
             );
/* MA END */

/* MACD BEGIN */
static const TA_OptInputParameterInfo TA_DEF_UI_D_MACD_SignalPeriod =
{
   TA_OptInput_IntegerRange,
   "optInSignalPeriod",
   0,

   "Signal Period",
   (const void *)&TA_DEF_TimePeriod_Positive,
   9,
   "Smoothing for the signal line (nb of period)",

   NULL
};

const TA_OutputParameterInfo TA_DEF_UI_Output_Real_MACD_outMACD =
                               { TA_Output_Real, "outMACD", TA_OUT_LINE };

const TA_OutputParameterInfo TA_DEF_UI_Output_Real_MACD_outMACDSignal =
                               { TA_Output_Real, "outMACDSignal", TA_OUT_DASH_LINE };

const TA_OutputParameterInfo TA_DEF_UI_Output_Real_MACD_outMACDHist =
                               { TA_Output_Real, "outMACDHist", TA_OUT_HISTO };

static const TA_InputParameterInfo    *TA_MACD_Inputs[]    =
{
  &TA_DEF_UI_Input_Real,
  NULL
};

static const TA_OutputParameterInfo   *TA_MACD_Outputs[]   =
{
  &TA_DEF_UI_Output_Real_MACD_outMACD,
  &TA_DEF_UI_Output_Real_MACD_outMACDSignal,
  &TA_DEF_UI_Output_Real_MACD_outMACDHist,
  NULL
};

static const TA_OptInputParameterInfo *TA_MACD_OptInputs[] =
{ &TA_DEF_UI_Fast_Period,
  &TA_DEF_UI_Slow_Period,
  &TA_DEF_UI_D_MACD_SignalPeriod,
  NULL
};

DEF_FUNCTION( MACD,
              TA_GroupId_MomentumIndicators,
              "Moving Average Convergence/Divergence",
              "Macd",
              TA_FUNC_FLG_STREAM
             );
/* MACD END */

/* MACDEXT BEGIN */
const TA_OptInputParameterInfo TA_DEF_UI_D_MACDEXT_FastMAType =
{
   TA_OptInput_IntegerList,
   "optInFastMAType",
   0,

   "Fast MA",
   (const void *)&TA_MA_TypeList,
   0,
   "Type of Moving Average for fast MA",

   NULL
};

const TA_OptInputParameterInfo TA_DEF_UI_D_MACDEXT_SlowMAType =
{
   TA_OptInput_IntegerList,
   "optInSlowMAType",
   0,

   "Slow MA",
   (const void *)&TA_MA_TypeList,
   0,
   "Type of Moving Average for slow MA",

   NULL
};

static const TA_OptInputParameterInfo TA_DEF_UI_D_MACDEXT_SignalPeriod =
{
   TA_OptInput_IntegerRange,
   "optInSignalPeriod",
   0,

   "Signal Period",
   (const void *)&TA_DEF_TimePeriod_Positive,
   9,
   "Smoothing for the signal line (nb of period)",

   NULL
};

const TA_OptInputParameterInfo TA_DEF_UI_D_MACDEXT_SignalMAType =
{
   TA_OptInput_IntegerList,
   "optInSignalMAType",
   0,

   "Signal MA",
   (const void *)&TA_MA_TypeList,
   0,
   "Type of Moving Average for signal line",

   NULL
};

const TA_OutputParameterInfo TA_DEF_UI_Output_Real_MACDEXT_outMACD =
                               { TA_Output_Real, "outMACD", TA_OUT_LINE };

const TA_OutputParameterInfo TA_DEF_UI_Output_Real_MACDEXT_outMACDSignal =
                               { TA_Output_Real, "outMACDSignal", TA_OUT_DASH_LINE };

const TA_OutputParameterInfo TA_DEF_UI_Output_Real_MACDEXT_outMACDHist =
                               { TA_Output_Real, "outMACDHist", TA_OUT_HISTO };

static const TA_InputParameterInfo    *TA_MACDEXT_Inputs[]    =
{
  &TA_DEF_UI_Input_Real,
  NULL
};

static const TA_OutputParameterInfo   *TA_MACDEXT_Outputs[]   =
{
  &TA_DEF_UI_Output_Real_MACDEXT_outMACD,
  &TA_DEF_UI_Output_Real_MACDEXT_outMACDSignal,
  &TA_DEF_UI_Output_Real_MACDEXT_outMACDHist,
  NULL
};

static const TA_OptInputParameterInfo *TA_MACDEXT_OptInputs[] =
{ &TA_DEF_UI_Fast_Period,
  &TA_DEF_UI_D_MACDEXT_FastMAType,
  &TA_DEF_UI_Slow_Period,
  &TA_DEF_UI_D_MACDEXT_SlowMAType,
  &TA_DEF_UI_D_MACDEXT_SignalPeriod,
  &TA_DEF_UI_D_MACDEXT_SignalMAType,
  NULL
};

DEF_FUNCTION( MACDEXT,
              TA_GroupId_MomentumIndicators,
              "MACD with controllable MA type",
              "MacdExt",
              0
             );
/* MACDEXT END */

/* MACDFIX BEGIN */
static const TA_OptInputParameterInfo TA_DEF_UI_D_MACDFIX_SignalPeriod =
{
   TA_OptInput_IntegerRange,
   "optInSignalPeriod",
   0,

   "Signal Period",
   (const void *)&TA_DEF_TimePeriod_Positive,
   9,
   "Smoothing for the signal line (nb of period)",

   NULL
};

const TA_OutputParameterInfo TA_DEF_UI_Output_Real_MACDFIX_outMACD =
                               { TA_Output_Real, "outMACD", TA_OUT_LINE };

const TA_OutputParameterInfo TA_DEF_UI_Output_Real_MACDFIX_outMACDSignal =
                               { TA_Output_Real, "outMACDSignal", TA_OUT_DASH_LINE };

const TA_OutputParameterInfo TA_DEF_UI_Output_Real_MACDFIX_outMACDHist =
                               { TA_Output_Real, "outMACDHist", TA_OUT_HISTO };

static const TA_InputParameterInfo    *TA_MACDFIX_Inputs[]    =
{
  &TA_DEF_UI_Input_Real,
  NULL
};

static const TA_OutputParameterInfo   *TA_MACDFIX_Outputs[]   =
{
  &TA_DEF_UI_Output_Real_MACDFIX_outMACD,
  &TA_DEF_UI_Output_Real_MACDFIX_outMACDSignal,
  &TA_DEF_UI_Output_Real_MACDFIX_outMACDHist,
  NULL
};

static const TA_OptInputParameterInfo *TA_MACDFIX_OptInputs[] =
{ &TA_DEF_UI_D_MACDFIX_SignalPeriod,
  NULL
};

DEF_FUNCTION( MACDFIX,
              TA_GroupId_MomentumIndicators,
              "Moving Average Convergence/Divergence Fix 12/26",
              "MacdFix",
              0
             );
/* MACDFIX END */

/* MAMA BEGIN */
static const TA_RealRange TA_DEF_MAMA_FastLimit =
{
   0.01,
   0.99,
   2,
   0.21,
   0.8,
   0.01
};

static const TA_OptInputParameterInfo TA_DEF_UI_D_MAMA_FastLimit =
{
   TA_OptInput_RealRange,
   "optInFastLimit",
   0,

   "Fast Limit",
   (const void *)&TA_DEF_MAMA_FastLimit,
   0.5,
   "Upper limit use in the adaptive algorithm",

   NULL
};

static const TA_RealRange TA_DEF_MAMA_SlowLimit =
{
   0.01,
   0.99,
   2,
   0.01,
   0.6,
   0.01
};

static const TA_OptInputParameterInfo TA_DEF_UI_D_MAMA_SlowLimit =
{
   TA_OptInput_RealRange,
   "optInSlowLimit",
   0,

   "Slow Limit",
   (const void *)&TA_DEF_MAMA_SlowLimit,
   0.05,
   "Lower limit use in the adaptive algorithm",

   NULL
};

const TA_OutputParameterInfo TA_DEF_UI_Output_Real_MAMA_outMAMA =
                               { TA_Output_Real, "outMAMA", TA_OUT_LINE };

const TA_OutputParameterInfo TA_DEF_UI_Output_Real_MAMA_outFAMA =
                               { TA_Output_Real, "outFAMA", TA_OUT_DASH_LINE };

static const TA_InputParameterInfo    *TA_MAMA_Inputs[]    =
{
  &TA_DEF_UI_Input_Real,
  NULL
};

static const TA_OutputParameterInfo   *TA_MAMA_Outputs[]   =
{
  &TA_DEF_UI_Output_Real_MAMA_outMAMA,
  &TA_DEF_UI_Output_Real_MAMA_outFAMA,
  NULL
};

static const TA_OptInputParameterInfo *TA_MAMA_OptInputs[] =
{ &TA_DEF_UI_D_MAMA_FastLimit,
  &TA_DEF_UI_D_MAMA_SlowLimit,
  NULL
};

DEF_FUNCTION( MAMA,
              TA_GroupId_OverlapStudies,
              "MESA Adaptive Moving Average",
              "Mama",
              TA_FUNC_FLG_UNST_PER | TA_FUNC_FLG_OVERLAP
             );
/* MAMA END */

/* MAVP BEGIN */
static const TA_OptInputParameterInfo TA_DEF_UI_D_MAVP_MinPeriod =
{
   TA_OptInput_IntegerRange,
   "optInMinPeriod",
   0,

   "Minimum Period",
   (const void *)&TA_DEF_TimePeriod_Positive,
   2,
   "Value less than minimum will be changed to Minimum period",

   NULL
};

static const TA_OptInputParameterInfo TA_DEF_UI_D_MAVP_MaxPeriod =
{
   TA_OptInput_IntegerRange,
   "optInMaxPeriod",
   0,

   "Maximum Period",
   (const void *)&TA_DEF_TimePeriod_Positive,
   30,
   "Value higher than maximum will be changed to Maximum period",

   NULL
};

static const TA_InputParameterInfo    *TA_MAVP_Inputs[]    =
{
  &TA_DEF_UI_Input_Real,
  &TA_DEF_UI_Input_Periods,
  NULL
};

static const TA_OutputParameterInfo   *TA_MAVP_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_MAVP_OptInputs[] =
{ &TA_DEF_UI_D_MAVP_MinPeriod,
  &TA_DEF_UI_D_MAVP_MaxPeriod,
  &TA_DEF_UI_MA_Method,
  NULL
};

DEF_FUNCTION( MAVP,
              TA_GroupId_OverlapStudies,
              "Moving average with variable period",
              "MovingAverageVariablePeriod",
              TA_FUNC_FLG_OVERLAP
             );
/* MAVP END */

/* MAX BEGIN */
static const TA_InputParameterInfo    *TA_MAX_Inputs[]    =
{
  &TA_DEF_UI_Input_Real,
  NULL
};

static const TA_OutputParameterInfo   *TA_MAX_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_MAX_OptInputs[] =
{ &TA_DEF_UI_TimePeriod_30_MINIMUM2,
  NULL
};

DEF_FUNCTION( MAX,
              TA_GroupId_MathOperators,
              "Highest value over a specified period",
              "Max",
              TA_FUNC_FLG_OVERLAP | TA_FUNC_FLG_STREAM
             );
/* MAX END */

/* MAXINDEX BEGIN */
static const TA_InputParameterInfo    *TA_MAXINDEX_Inputs[]    =
{
  &TA_DEF_UI_Input_Real,
  NULL
};

static const TA_OutputParameterInfo   *TA_MAXINDEX_Outputs[]   =
{
  &TA_DEF_UI_Output_Integer,
  NULL
};

static const TA_OptInputParameterInfo *TA_MAXINDEX_OptInputs[] =
{ &TA_DEF_UI_TimePeriod_30_MINIMUM2,
  NULL
};

DEF_FUNCTION( MAXINDEX,
              TA_GroupId_MathOperators,
              "Index of highest value over a specified period",
              "MaxIndex",
              TA_FUNC_FLG_STREAM
             );
/* MAXINDEX END */

/* MEDPRICE BEGIN */
static const TA_InputParameterInfo    *TA_MEDPRICE_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_HL,
  NULL
};

static const TA_OutputParameterInfo   *TA_MEDPRICE_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_MEDPRICE_OptInputs[] =
{ NULL };

DEF_FUNCTION( MEDPRICE,
              TA_GroupId_PriceTransform,
              "Median Price",
              "MedPrice",
              TA_FUNC_FLG_OVERLAP | TA_FUNC_FLG_STREAM
             );
/* MEDPRICE END */

/* MFI BEGIN */
static const TA_InputParameterInfo    *TA_MFI_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_HLCV,
  NULL
};

static const TA_OutputParameterInfo   *TA_MFI_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_MFI_OptInputs[] =
{ &TA_DEF_UI_TimePeriod_14_MINIMUM2,
  NULL
};

DEF_FUNCTION( MFI,
              TA_GroupId_MomentumIndicators,
              "Money Flow Index",
              "Mfi",
              TA_FUNC_FLG_STREAM
             );
/* MFI END */

/* MIDPOINT BEGIN */
static const TA_InputParameterInfo    *TA_MIDPOINT_Inputs[]    =
{
  &TA_DEF_UI_Input_Real,
  NULL
};

static const TA_OutputParameterInfo   *TA_MIDPOINT_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_MIDPOINT_OptInputs[] =
{ &TA_DEF_UI_TimePeriod_14_MINIMUM2,
  NULL
};

DEF_FUNCTION( MIDPOINT,
              TA_GroupId_OverlapStudies,
              "MidPoint over period",
              "MidPoint",
              TA_FUNC_FLG_OVERLAP | TA_FUNC_FLG_STREAM
             );
/* MIDPOINT END */

/* MIDPRICE BEGIN */
static const TA_InputParameterInfo    *TA_MIDPRICE_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_HL,
  NULL
};

static const TA_OutputParameterInfo   *TA_MIDPRICE_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_MIDPRICE_OptInputs[] =
{ &TA_DEF_UI_TimePeriod_14_MINIMUM2,
  NULL
};

DEF_FUNCTION( MIDPRICE,
              TA_GroupId_OverlapStudies,
              "Midpoint Price over period",
              "MidPrice",
              TA_FUNC_FLG_OVERLAP
             );
/* MIDPRICE END */

/* MIN BEGIN */
static const TA_InputParameterInfo    *TA_MIN_Inputs[]    =
{
  &TA_DEF_UI_Input_Real,
  NULL
};

static const TA_OutputParameterInfo   *TA_MIN_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_MIN_OptInputs[] =
{ &TA_DEF_UI_TimePeriod_30_MINIMUM2,
  NULL
};

DEF_FUNCTION( MIN,
              TA_GroupId_MathOperators,
              "Lowest value over a specified period",
              "Min",
              TA_FUNC_FLG_OVERLAP | TA_FUNC_FLG_STREAM
             );
/* MIN END */

/* MININDEX BEGIN */
static const TA_InputParameterInfo    *TA_MININDEX_Inputs[]    =
{
  &TA_DEF_UI_Input_Real,
  NULL
};

static const TA_OutputParameterInfo   *TA_MININDEX_Outputs[]   =
{
  &TA_DEF_UI_Output_Integer,
  NULL
};

static const TA_OptInputParameterInfo *TA_MININDEX_OptInputs[] =
{ &TA_DEF_UI_TimePeriod_30_MINIMUM2,
  NULL
};

DEF_FUNCTION( MININDEX,
              TA_GroupId_MathOperators,
              "Index of lowest value over a specified period",
              "MinIndex",
              TA_FUNC_FLG_STREAM
             );
/* MININDEX END */

/* MINMAX BEGIN */
const TA_OutputParameterInfo TA_DEF_UI_Output_Real_MINMAX_outMin =
                               { TA_Output_Real, "outMin", TA_OUT_LINE };

const TA_OutputParameterInfo TA_DEF_UI_Output_Real_MINMAX_outMax =
                               { TA_Output_Real, "outMax", TA_OUT_LINE };

static const TA_InputParameterInfo    *TA_MINMAX_Inputs[]    =
{
  &TA_DEF_UI_Input_Real,
  NULL
};

static const TA_OutputParameterInfo   *TA_MINMAX_Outputs[]   =
{
  &TA_DEF_UI_Output_Real_MINMAX_outMin,
  &TA_DEF_UI_Output_Real_MINMAX_outMax,
  NULL
};

static const TA_OptInputParameterInfo *TA_MINMAX_OptInputs[] =
{ &TA_DEF_UI_TimePeriod_30_MINIMUM2,
  NULL
};

DEF_FUNCTION( MINMAX,
              TA_GroupId_MathOperators,
              "Lowest and highest values over a specified period",
              "MinMax",
              TA_FUNC_FLG_OVERLAP | TA_FUNC_FLG_STREAM
             );
/* MINMAX END */

/* MINMAXINDEX BEGIN */
const TA_OutputParameterInfo TA_DEF_UI_Output_Integer_MINMAXINDEX_outMinIdx =
                               { TA_Output_Integer, "outMinIdx", TA_OUT_LINE };

const TA_OutputParameterInfo TA_DEF_UI_Output_Integer_MINMAXINDEX_outMaxIdx =
                               { TA_Output_Integer, "outMaxIdx", TA_OUT_LINE };

static const TA_InputParameterInfo    *TA_MINMAXINDEX_Inputs[]    =
{
  &TA_DEF_UI_Input_Real,
  NULL
};

static const TA_OutputParameterInfo   *TA_MINMAXINDEX_Outputs[]   =
{
  &TA_DEF_UI_Output_Integer_MINMAXINDEX_outMinIdx,
  &TA_DEF_UI_Output_Integer_MINMAXINDEX_outMaxIdx,
  NULL
};

static const TA_OptInputParameterInfo *TA_MINMAXINDEX_OptInputs[] =
{ &TA_DEF_UI_TimePeriod_30_MINIMUM2,
  NULL
};

DEF_FUNCTION( MINMAXINDEX,
              TA_GroupId_MathOperators,
              "Indexes of lowest and highest values over a specified period",
              "MinMaxIndex",
              TA_FUNC_FLG_STREAM
             );
/* MINMAXINDEX END */

/* MINUS_DI BEGIN */
static const TA_InputParameterInfo    *TA_MINUS_DI_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_HLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_MINUS_DI_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_MINUS_DI_OptInputs[] =
{ &TA_DEF_UI_TimePeriod_14,
  NULL
};

DEF_FUNCTION( MINUS_DI,
              TA_GroupId_MomentumIndicators,
              "Minus Directional Indicator",
              "MinusDI",
              TA_FUNC_FLG_UNST_PER
             );
/* MINUS_DI END */

/* MINUS_DM BEGIN */
static const TA_InputParameterInfo    *TA_MINUS_DM_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_HL,
  NULL
};

static const TA_OutputParameterInfo   *TA_MINUS_DM_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_MINUS_DM_OptInputs[] =
{ &TA_DEF_UI_TimePeriod_14,
  NULL
};

DEF_FUNCTION( MINUS_DM,
              TA_GroupId_MomentumIndicators,
              "Minus Directional Movement",
              "MinusDM",
              TA_FUNC_FLG_UNST_PER
             );
/* MINUS_DM END */

/* MOM BEGIN */
static const TA_InputParameterInfo    *TA_MOM_Inputs[]    =
{
  &TA_DEF_UI_Input_Real,
  NULL
};

static const TA_OutputParameterInfo   *TA_MOM_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_MOM_OptInputs[] =
{ &TA_DEF_UI_TimePeriod_10,
  NULL
};

DEF_FUNCTION( MOM,
              TA_GroupId_MomentumIndicators,
              "Momentum",
              "Mom",
              TA_FUNC_FLG_STREAM
             );
/* MOM END */

/* MULT BEGIN */
static const TA_InputParameterInfo    *TA_MULT_Inputs[]    =
{
  &TA_DEF_UI_Input_Real0,
  &TA_DEF_UI_Input_Real1,
  NULL
};

static const TA_OutputParameterInfo   *TA_MULT_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_MULT_OptInputs[] =
{ NULL };

DEF_FUNCTION( MULT,
              TA_GroupId_MathOperators,
              "Vector Arithmetic Mult",
              "Mult",
              TA_FUNC_FLG_STREAM
             );
/* MULT END */

/****************************************************************************
 * Step 2 - Add your TA function to the table.
 *          Keep in alphabetical order. Must be NULL terminated.
 ****************************************************************************/
const TA_FuncDef *TA_DEF_TableM[] =
{
   ADD_TO_TABLE(MA),
   ADD_TO_TABLE(MACD),
   ADD_TO_TABLE(MACDEXT),
   ADD_TO_TABLE(MACDFIX),
   ADD_TO_TABLE(MAMA),
   ADD_TO_TABLE(MAVP),
   ADD_TO_TABLE(MAX),
   ADD_TO_TABLE(MAXINDEX),
   ADD_TO_TABLE(MEDPRICE),
   ADD_TO_TABLE(MFI),
   ADD_TO_TABLE(MIDPOINT),
   ADD_TO_TABLE(MIDPRICE),
   ADD_TO_TABLE(MIN),
   ADD_TO_TABLE(MININDEX),
   ADD_TO_TABLE(MINMAX),
   ADD_TO_TABLE(MINMAXINDEX),
   ADD_TO_TABLE(MINUS_DI),
   ADD_TO_TABLE(MINUS_DM),
   ADD_TO_TABLE(MOM),
   ADD_TO_TABLE(MULT),
   NULL
};


/* Do not modify the following line. */
const unsigned int TA_DEF_TableMSize =
              ((sizeof(TA_DEF_TableM)/sizeof(TA_FuncDef *))-1);

