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

/*********************************************************************
 * This file contains only TA functions starting with the letter 'S' *
 *********************************************************************/
#include <stddef.h>
#include "ta_abstract.h"
#include "ta_def_ui.h"

/* SAR BEGIN */
static const TA_RealRange TA_DEF_SAR_Acceleration =
{
   0.0,
   TA_REAL_MAX,
   4,
   0.01,
   0.2,
   0.01
};

static const TA_OptInputParameterInfo TA_DEF_UI_D_SAR_Acceleration =
{
   TA_OptInput_RealRange,
   "optInAcceleration",
   0,

   "Acceleration Factor",
   (const void *)&TA_DEF_SAR_Acceleration,
   0.02,
   "Acceleration Factor used up to the Maximum value",

   NULL
};

static const TA_RealRange TA_DEF_SAR_Maximum =
{
   0.0,
   TA_REAL_MAX,
   4,
   0.2,
   0.4,
   0.01
};

static const TA_OptInputParameterInfo TA_DEF_UI_D_SAR_Maximum =
{
   TA_OptInput_RealRange,
   "optInMaximum",
   0,

   "AF Maximum",
   (const void *)&TA_DEF_SAR_Maximum,
   0.2,
   "Acceleration Factor Maximum value",

   NULL
};

static const TA_InputParameterInfo    *TA_SAR_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_HL,
  NULL
};

static const TA_OutputParameterInfo   *TA_SAR_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_SAR_OptInputs[] =
{ &TA_DEF_UI_D_SAR_Acceleration,
  &TA_DEF_UI_D_SAR_Maximum,
  NULL
};

DEF_FUNCTION( SAR,
              TA_GroupId_OverlapStudies,
              "Parabolic SAR",
              TA_FUNC_FLG_OVERLAP | TA_FUNC_FLG_STREAM | TA_FUNC_FLG_PATH_DEP
             );
/* SAR END */

/* SAREXT BEGIN */
static const TA_RealRange TA_DEF_SAREXT_StartValue =
{
   TA_REAL_MIN,
   TA_REAL_MAX,
   4,
   0.0,
   0.0,
   0.0
};

static const TA_OptInputParameterInfo TA_DEF_UI_D_SAREXT_StartValue =
{
   TA_OptInput_RealRange,
   "optInStartValue",
   0,

   "Start Value",
   (const void *)&TA_DEF_SAREXT_StartValue,
   0.0,
   "Start value and direction. 0 for Auto, >0 for Long, <0 for Short",

   NULL
};

static const TA_RealRange TA_DEF_SAREXT_OffsetOnReverse =
{
   0.0,
   TA_REAL_MAX,
   4,
   0.01,
   0.15,
   0.01
};

static const TA_OptInputParameterInfo TA_DEF_UI_D_SAREXT_OffsetOnReverse =
{
   TA_OptInput_RealRange,
   "optInOffsetOnReverse",
   0,

   "Offset on Reverse",
   (const void *)&TA_DEF_SAREXT_OffsetOnReverse,
   0.0,
   "Percent offset added/removed to initial stop on short/long reversal",

   NULL
};

static const TA_RealRange TA_DEF_SAREXT_AccelerationInitLong =
{
   0.0,
   TA_REAL_MAX,
   4,
   0.01,
   0.19,
   0.01
};

static const TA_OptInputParameterInfo TA_DEF_UI_D_SAREXT_AccelerationInitLong =
{
   TA_OptInput_RealRange,
   "optInAccelerationInitLong",
   0,

   "AF Init Long",
   (const void *)&TA_DEF_SAREXT_AccelerationInitLong,
   0.02,
   "Acceleration Factor initial value for the Long direction",

   NULL
};

static const TA_RealRange TA_DEF_SAREXT_AccelerationLong =
{
   0.0,
   TA_REAL_MAX,
   4,
   0.01,
   0.2,
   0.01
};

static const TA_OptInputParameterInfo TA_DEF_UI_D_SAREXT_AccelerationLong =
{
   TA_OptInput_RealRange,
   "optInAccelerationLong",
   0,

   "AF Long",
   (const void *)&TA_DEF_SAREXT_AccelerationLong,
   0.02,
   "Acceleration Factor for the Long direction",

   NULL
};

static const TA_RealRange TA_DEF_SAREXT_AccelerationMaxLong =
{
   0.0,
   TA_REAL_MAX,
   4,
   0.2,
   0.4,
   0.01
};

static const TA_OptInputParameterInfo TA_DEF_UI_D_SAREXT_AccelerationMaxLong =
{
   TA_OptInput_RealRange,
   "optInAccelerationMaxLong",
   0,

   "AF Max Long",
   (const void *)&TA_DEF_SAREXT_AccelerationMaxLong,
   0.2,
   "Acceleration Factor maximum value for the Long direction",

   NULL
};

static const TA_RealRange TA_DEF_SAREXT_AccelerationInitShort =
{
   0.0,
   TA_REAL_MAX,
   4,
   0.01,
   0.19,
   0.01
};

static const TA_OptInputParameterInfo TA_DEF_UI_D_SAREXT_AccelerationInitShort =
{
   TA_OptInput_RealRange,
   "optInAccelerationInitShort",
   0,

   "AF Init Short",
   (const void *)&TA_DEF_SAREXT_AccelerationInitShort,
   0.02,
   "Acceleration Factor initial value for the Short direction",

   NULL
};

static const TA_RealRange TA_DEF_SAREXT_AccelerationShort =
{
   0.0,
   TA_REAL_MAX,
   4,
   0.01,
   0.2,
   0.01
};

static const TA_OptInputParameterInfo TA_DEF_UI_D_SAREXT_AccelerationShort =
{
   TA_OptInput_RealRange,
   "optInAccelerationShort",
   0,

   "AF Short",
   (const void *)&TA_DEF_SAREXT_AccelerationShort,
   0.02,
   "Acceleration Factor for the Short direction",

   NULL
};

static const TA_RealRange TA_DEF_SAREXT_AccelerationMaxShort =
{
   0.0,
   TA_REAL_MAX,
   4,
   0.2,
   0.4,
   0.01
};

static const TA_OptInputParameterInfo TA_DEF_UI_D_SAREXT_AccelerationMaxShort =
{
   TA_OptInput_RealRange,
   "optInAccelerationMaxShort",
   0,

   "AF Max Short",
   (const void *)&TA_DEF_SAREXT_AccelerationMaxShort,
   0.2,
   "Acceleration Factor maximum value for the Short direction",

   NULL
};

static const TA_InputParameterInfo    *TA_SAREXT_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_HL,
  NULL
};

static const TA_OutputParameterInfo   *TA_SAREXT_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_SAREXT_OptInputs[] =
{ &TA_DEF_UI_D_SAREXT_StartValue,
  &TA_DEF_UI_D_SAREXT_OffsetOnReverse,
  &TA_DEF_UI_D_SAREXT_AccelerationInitLong,
  &TA_DEF_UI_D_SAREXT_AccelerationLong,
  &TA_DEF_UI_D_SAREXT_AccelerationMaxLong,
  &TA_DEF_UI_D_SAREXT_AccelerationInitShort,
  &TA_DEF_UI_D_SAREXT_AccelerationShort,
  &TA_DEF_UI_D_SAREXT_AccelerationMaxShort,
  NULL
};

DEF_FUNCTION( SAREXT,
              TA_GroupId_OverlapStudies,
              "Parabolic SAR - Extended",
              TA_FUNC_FLG_OVERLAP | TA_FUNC_FLG_STREAM | TA_FUNC_FLG_PATH_DEP
             );
/* SAREXT END */

/* SI BEGIN */
static const TA_RealRange TA_DEF_SI_LimitMove =
{
   0.00000001,
   TA_REAL_MAX,
   4,
   0.5,
   30.0,
   0.5
};

static const TA_OptInputParameterInfo TA_DEF_UI_D_SI_LimitMove =
{
   TA_OptInput_RealRange,
   "optInLimitMove",
   0,

   "Limit Move",
   (const void *)&TA_DEF_SI_LimitMove,
   3.0,
   "Largest one-bar price move the index is scaled against, in price units",

   NULL
};

static const TA_InputParameterInfo    *TA_SI_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_OHLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_SI_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_SI_OptInputs[] =
{ &TA_DEF_UI_D_SI_LimitMove,
  NULL
};

DEF_FUNCTION( SI,
              TA_GroupId_MomentumIndicators,
              "Wilder Swing Index",
              TA_FUNC_FLG_STREAM
             );
/* SI END */

/* SIN BEGIN */
static const TA_InputParameterInfo    *TA_SIN_Inputs[]    =
{
  &TA_DEF_UI_Input_Real,
  NULL
};

static const TA_OutputParameterInfo   *TA_SIN_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_SIN_OptInputs[] =
{ NULL };

DEF_FUNCTION( SIN,
              TA_GroupId_MathTransform,
              "Vector Trigonometric Sin",
              TA_FUNC_FLG_STREAM
             );
/* SIN END */

/* SINH BEGIN */
static const TA_InputParameterInfo    *TA_SINH_Inputs[]    =
{
  &TA_DEF_UI_Input_Real,
  NULL
};

static const TA_OutputParameterInfo   *TA_SINH_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_SINH_OptInputs[] =
{ NULL };

DEF_FUNCTION( SINH,
              TA_GroupId_MathTransform,
              "Vector Trigonometric Sinh",
              TA_FUNC_FLG_STREAM | TA_FUNC_FLG_NAN_INF_OUT
             );
/* SINH END */

/* SMA BEGIN */
static const TA_InputParameterInfo    *TA_SMA_Inputs[]    =
{
  &TA_DEF_UI_Input_Real,
  NULL
};

static const TA_OutputParameterInfo   *TA_SMA_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_SMA_OptInputs[] =
{ &TA_DEF_UI_TimePeriod_30,
  NULL
};

DEF_FUNCTION( SMA,
              TA_GroupId_OverlapStudies,
              "Simple Moving Average",
              TA_FUNC_FLG_OVERLAP | TA_FUNC_FLG_STREAM | TA_FUNC_FLG_PERIOD1_IDENTITY
             );
/* SMA END */

/* SMI BEGIN */
static const TA_OptInputParameterInfo TA_DEF_UI_D_SMI_TimePeriod =
{
   TA_OptInput_IntegerRange,
   "optInTimePeriod",
   0,

   "Time Period",
   (const void *)&TA_DEF_TimePeriod_Positive_Minimum2,
   13,
   "Period of the high/low range",

   NULL
};

static const TA_IntegerRange TA_DEF_SMI_FastPeriod =
{
   2,
   100000,
   2,
   200,
   1
};

static const TA_OptInputParameterInfo TA_DEF_UI_D_SMI_FastPeriod =
{
   TA_OptInput_IntegerRange,
   "optInFastPeriod",
   0,

   "Fast Period",
   (const void *)&TA_DEF_SMI_FastPeriod,
   2,
   "Period of the second smoothing, applied to the first",

   NULL
};

static const TA_IntegerRange TA_DEF_SMI_SlowPeriod =
{
   2,
   100000,
   2,
   200,
   1
};

static const TA_OptInputParameterInfo TA_DEF_UI_D_SMI_SlowPeriod =
{
   TA_OptInput_IntegerRange,
   "optInSlowPeriod",
   0,

   "Slow Period",
   (const void *)&TA_DEF_SMI_SlowPeriod,
   25,
   "Period of the first smoothing, applied to the raw momentum",

   NULL
};

static const TA_IntegerRange TA_DEF_SMI_SignalPeriod =
{
   2,
   100000,
   2,
   200,
   1
};

static const TA_OptInputParameterInfo TA_DEF_UI_D_SMI_SignalPeriod =
{
   TA_OptInput_IntegerRange,
   "optInSignalPeriod",
   0,

   "Signal Period",
   (const void *)&TA_DEF_SMI_SignalPeriod,
   9,
   "Smoothing for the signal line (period length)",

   NULL
};

const TA_OutputParameterInfo TA_DEF_UI_Output_Real_SMI_outSMI =
                               { TA_Output_Real, "outSMI", TA_OUT_LINE };

const TA_OutputParameterInfo TA_DEF_UI_Output_Real_SMI_outSMISignal =
                               { TA_Output_Real, "outSMISignal", TA_OUT_DASH_LINE };

static const TA_InputParameterInfo    *TA_SMI_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_HLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_SMI_Outputs[]   =
{
  &TA_DEF_UI_Output_Real_SMI_outSMI,
  &TA_DEF_UI_Output_Real_SMI_outSMISignal,
  NULL
};

static const TA_OptInputParameterInfo *TA_SMI_OptInputs[] =
{ &TA_DEF_UI_D_SMI_TimePeriod,
  &TA_DEF_UI_D_SMI_FastPeriod,
  &TA_DEF_UI_D_SMI_SlowPeriod,
  &TA_DEF_UI_D_SMI_SignalPeriod,
  NULL
};

DEF_FUNCTION( SMI,
              TA_GroupId_MomentumIndicators,
              "Stochastic Momentum Index",
              TA_FUNC_FLG_STREAM
             );
/* SMI END */

/* SQRT BEGIN */
static const TA_InputParameterInfo    *TA_SQRT_Inputs[]    =
{
  &TA_DEF_UI_Input_Real,
  NULL
};

static const TA_OutputParameterInfo   *TA_SQRT_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_SQRT_OptInputs[] =
{ NULL };

DEF_FUNCTION( SQRT,
              TA_GroupId_MathTransform,
              "Vector Square Root",
              TA_FUNC_FLG_STREAM | TA_FUNC_FLG_NAN_INF_OUT
             );
/* SQRT END */

/* SQZMOM BEGIN */
static const TA_OptInputParameterInfo TA_DEF_UI_D_SQZMOM_BBPeriod =
{
   TA_OptInput_IntegerRange,
   "optInBBPeriod",
   0,

   "BB Period",
   (const void *)&TA_DEF_TimePeriod_Positive_Minimum2,
   20,
   "Period of the Bollinger Bands",

   NULL
};

static const TA_RealRange TA_DEF_SQZMOM_NbDev =
{
   0.0,
   TA_REAL_MAX,
   2,
   1.0,
   3.0,
   0.2
};

static const TA_OptInputParameterInfo TA_DEF_UI_D_SQZMOM_NbDev =
{
   TA_OptInput_RealRange,
   "optInNbDev",
   0,

   "Deviations",
   (const void *)&TA_DEF_SQZMOM_NbDev,
   2.0,
   "Deviation multiplier for both Bollinger Bands",

   NULL
};

static const TA_OptInputParameterInfo TA_DEF_UI_D_SQZMOM_KCPeriod =
{
   TA_OptInput_IntegerRange,
   "optInKCPeriod",
   0,

   "KC Period",
   (const void *)&TA_DEF_TimePeriod_Positive_Minimum2,
   20,
   "Period of the Keltner Channel, and of the momentum regression",

   NULL
};

static const TA_RealRange TA_DEF_SQZMOM_FactorWide =
{
   0.0,
   TA_REAL_MAX,
   2,
   1.0,
   3.0,
   0.25
};

static const TA_OptInputParameterInfo TA_DEF_UI_D_SQZMOM_FactorWide =
{
   TA_OptInput_RealRange,
   "optInFactorWide",
   0,

   "Wide Factor",
   (const void *)&TA_DEF_SQZMOM_FactorWide,
   2.0,
   "Keltner width for the widest compression test",

   NULL
};

static const TA_RealRange TA_DEF_SQZMOM_FactorNormal =
{
   0.0,
   TA_REAL_MAX,
   2,
   1.0,
   3.0,
   0.25
};

static const TA_OptInputParameterInfo TA_DEF_UI_D_SQZMOM_FactorNormal =
{
   TA_OptInput_RealRange,
   "optInFactorNormal",
   0,

   "Normal Factor",
   (const void *)&TA_DEF_SQZMOM_FactorNormal,
   1.5,
   "Keltner width for the classic squeeze test",

   NULL
};

static const TA_RealRange TA_DEF_SQZMOM_FactorNarrow =
{
   0.0,
   TA_REAL_MAX,
   2,
   0.5,
   2.0,
   0.25
};

static const TA_OptInputParameterInfo TA_DEF_UI_D_SQZMOM_FactorNarrow =
{
   TA_OptInput_RealRange,
   "optInFactorNarrow",
   0,

   "Narrow Factor",
   (const void *)&TA_DEF_SQZMOM_FactorNarrow,
   1.0,
   "Keltner width for the tightest compression test",

   NULL
};

const TA_OutputParameterInfo TA_DEF_UI_Output_Real_SQZMOM_outMomentum =
                               { TA_Output_Real, "outMomentum", TA_OUT_LINE };

const TA_OutputParameterInfo TA_DEF_UI_Output_Integer_SQZMOM_outSqueeze =
                               { TA_Output_Integer, "outSqueeze", TA_OUT_LINE };

static const TA_InputParameterInfo    *TA_SQZMOM_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_HLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_SQZMOM_Outputs[]   =
{
  &TA_DEF_UI_Output_Real_SQZMOM_outMomentum,
  &TA_DEF_UI_Output_Integer_SQZMOM_outSqueeze,
  NULL
};

static const TA_OptInputParameterInfo *TA_SQZMOM_OptInputs[] =
{ &TA_DEF_UI_D_SQZMOM_BBPeriod,
  &TA_DEF_UI_D_SQZMOM_NbDev,
  &TA_DEF_UI_D_SQZMOM_KCPeriod,
  &TA_DEF_UI_D_SQZMOM_FactorWide,
  &TA_DEF_UI_D_SQZMOM_FactorNormal,
  &TA_DEF_UI_D_SQZMOM_FactorNarrow,
  NULL
};

DEF_FUNCTION( SQZMOM,
              TA_GroupId_MomentumIndicators,
              "Squeeze Momentum and Level",
              TA_FUNC_FLG_STREAM
             );
/* SQZMOM END */

/* STC BEGIN */
static const TA_OptInputParameterInfo TA_DEF_UI_D_STC_FastPeriod =
{
   TA_OptInput_IntegerRange,
   "optInFastPeriod",
   0,

   "Fast Period",
   (const void *)&TA_DEF_TimePeriod_Positive_Minimum2,
   23,
   "Period of the fast EMA",

   NULL
};

static const TA_OptInputParameterInfo TA_DEF_UI_D_STC_SlowPeriod =
{
   TA_OptInput_IntegerRange,
   "optInSlowPeriod",
   0,

   "Slow Period",
   (const void *)&TA_DEF_TimePeriod_Positive_Minimum2,
   50,
   "Period of the slow EMA",

   NULL
};

static const TA_IntegerRange TA_DEF_STC_CyclePeriod =
{
   2,
   100000,
   2,
   200,
   1
};

static const TA_OptInputParameterInfo TA_DEF_UI_D_STC_CyclePeriod =
{
   TA_OptInput_IntegerRange,
   "optInCyclePeriod",
   0,

   "Cycle Period",
   (const void *)&TA_DEF_STC_CyclePeriod,
   10,
   "Window of both stochastic stages",

   NULL
};

static const TA_InputParameterInfo    *TA_STC_Inputs[]    =
{
  &TA_DEF_UI_Input_Real,
  NULL
};

static const TA_OutputParameterInfo   *TA_STC_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_STC_OptInputs[] =
{ &TA_DEF_UI_D_STC_FastPeriod,
  &TA_DEF_UI_D_STC_SlowPeriod,
  &TA_DEF_UI_D_STC_CyclePeriod,
  NULL
};

DEF_FUNCTION( STC,
              TA_GroupId_MomentumIndicators,
              "Schaff Trend Cycle",
              TA_FUNC_FLG_UNST_PER | TA_FUNC_FLG_STREAM
             );
/* STC END */

/* STDDEV BEGIN */
static const TA_InputParameterInfo    *TA_STDDEV_Inputs[]    =
{
  &TA_DEF_UI_Input_Real,
  NULL
};

static const TA_OutputParameterInfo   *TA_STDDEV_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_STDDEV_OptInputs[] =
{ &TA_DEF_UI_TimePeriod_5_MINIMUM2,
  &TA_DEF_UI_NbDeviation,
  NULL
};

DEF_FUNCTION( STDDEV,
              TA_GroupId_Statistic,
              "Standard Deviation",
              TA_FUNC_FLG_STREAM
             );
/* STDDEV END */

/* STOCH BEGIN */
static const TA_OptInputParameterInfo TA_DEF_UI_D_STOCH_FastK_Period =
{
   TA_OptInput_IntegerRange,
   "optInFastK_Period",
   0,

   "Fast-K Period",
   (const void *)&TA_DEF_TimePeriod_Positive,
   5,
   "Time period for building the Fast-K line",

   NULL
};

static const TA_OptInputParameterInfo TA_DEF_UI_D_STOCH_SlowK_Period =
{
   TA_OptInput_IntegerRange,
   "optInSlowK_Period",
   0,

   "Slow-K Period",
   (const void *)&TA_DEF_TimePeriod_Positive,
   3,
   "Smoothing for making the Slow-K line. Usually set to 3",

   NULL
};

const TA_OptInputParameterInfo TA_DEF_UI_D_STOCH_SlowK_MAType =
{
   TA_OptInput_IntegerList,
   "optInSlowK_MAType",
   0,

   "Slow-K MA",
   (const void *)&TA_MA_TypeList,
   0,
   "Type of Moving Average for Slow-K",

   NULL
};

static const TA_OptInputParameterInfo TA_DEF_UI_D_STOCH_SlowD_Period =
{
   TA_OptInput_IntegerRange,
   "optInSlowD_Period",
   0,

   "Slow-D Period",
   (const void *)&TA_DEF_TimePeriod_Positive,
   3,
   "Smoothing for making the Slow-D line",

   NULL
};

const TA_OptInputParameterInfo TA_DEF_UI_D_STOCH_SlowD_MAType =
{
   TA_OptInput_IntegerList,
   "optInSlowD_MAType",
   0,

   "Slow-D MA",
   (const void *)&TA_MA_TypeList,
   0,
   "Type of Moving Average for Slow-D",

   NULL
};

const TA_OutputParameterInfo TA_DEF_UI_Output_Real_STOCH_outSlowK =
                               { TA_Output_Real, "outSlowK", TA_OUT_DASH_LINE };

const TA_OutputParameterInfo TA_DEF_UI_Output_Real_STOCH_outSlowD =
                               { TA_Output_Real, "outSlowD", TA_OUT_DASH_LINE };

static const TA_InputParameterInfo    *TA_STOCH_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_HLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_STOCH_Outputs[]   =
{
  &TA_DEF_UI_Output_Real_STOCH_outSlowK,
  &TA_DEF_UI_Output_Real_STOCH_outSlowD,
  NULL
};

static const TA_OptInputParameterInfo *TA_STOCH_OptInputs[] =
{ &TA_DEF_UI_D_STOCH_FastK_Period,
  &TA_DEF_UI_D_STOCH_SlowK_Period,
  &TA_DEF_UI_D_STOCH_SlowK_MAType,
  &TA_DEF_UI_D_STOCH_SlowD_Period,
  &TA_DEF_UI_D_STOCH_SlowD_MAType,
  NULL
};

DEF_FUNCTION( STOCH,
              TA_GroupId_MomentumIndicators,
              "Stochastic",
              TA_FUNC_FLG_STREAM
             );
/* STOCH END */

/* STOCHF BEGIN */
static const TA_OptInputParameterInfo TA_DEF_UI_D_STOCHF_FastK_Period =
{
   TA_OptInput_IntegerRange,
   "optInFastK_Period",
   0,

   "Fast-K Period",
   (const void *)&TA_DEF_TimePeriod_Positive,
   5,
   "Time period for building the Fast-K line",

   NULL
};

static const TA_OptInputParameterInfo TA_DEF_UI_D_STOCHF_FastD_Period =
{
   TA_OptInput_IntegerRange,
   "optInFastD_Period",
   0,

   "Fast-D Period",
   (const void *)&TA_DEF_TimePeriod_Positive,
   3,
   "Smoothing for making the Fast-D line. Usually set to 3",

   NULL
};

const TA_OptInputParameterInfo TA_DEF_UI_D_STOCHF_FastD_MAType =
{
   TA_OptInput_IntegerList,
   "optInFastD_MAType",
   0,

   "Fast-D MA",
   (const void *)&TA_MA_TypeList,
   0,
   "Type of Moving Average for Fast-D",

   NULL
};

const TA_OutputParameterInfo TA_DEF_UI_Output_Real_STOCHF_outFastK =
                               { TA_Output_Real, "outFastK", TA_OUT_LINE };

const TA_OutputParameterInfo TA_DEF_UI_Output_Real_STOCHF_outFastD =
                               { TA_Output_Real, "outFastD", TA_OUT_LINE };

static const TA_InputParameterInfo    *TA_STOCHF_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_HLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_STOCHF_Outputs[]   =
{
  &TA_DEF_UI_Output_Real_STOCHF_outFastK,
  &TA_DEF_UI_Output_Real_STOCHF_outFastD,
  NULL
};

static const TA_OptInputParameterInfo *TA_STOCHF_OptInputs[] =
{ &TA_DEF_UI_D_STOCHF_FastK_Period,
  &TA_DEF_UI_D_STOCHF_FastD_Period,
  &TA_DEF_UI_D_STOCHF_FastD_MAType,
  NULL
};

DEF_FUNCTION( STOCHF,
              TA_GroupId_MomentumIndicators,
              "Stochastic Fast",
              TA_FUNC_FLG_STREAM
             );
/* STOCHF END */

/* STOCHRSI BEGIN */
static const TA_OptInputParameterInfo TA_DEF_UI_D_STOCHRSI_FastK_Period =
{
   TA_OptInput_IntegerRange,
   "optInFastK_Period",
   0,

   "Fast-K Period",
   (const void *)&TA_DEF_TimePeriod_Positive,
   5,
   "Time period for building the Fast-K line",

   NULL
};

static const TA_OptInputParameterInfo TA_DEF_UI_D_STOCHRSI_FastD_Period =
{
   TA_OptInput_IntegerRange,
   "optInFastD_Period",
   0,

   "Fast-D Period",
   (const void *)&TA_DEF_TimePeriod_Positive,
   3,
   "Smoothing for making the Fast-D line. Usually set to 3",

   NULL
};

const TA_OptInputParameterInfo TA_DEF_UI_D_STOCHRSI_FastD_MAType =
{
   TA_OptInput_IntegerList,
   "optInFastD_MAType",
   0,

   "Fast-D MA",
   (const void *)&TA_MA_TypeList,
   0,
   "Type of Moving Average for Fast-D",

   NULL
};

const TA_OutputParameterInfo TA_DEF_UI_Output_Real_STOCHRSI_outFastK =
                               { TA_Output_Real, "outFastK", TA_OUT_LINE };

const TA_OutputParameterInfo TA_DEF_UI_Output_Real_STOCHRSI_outFastD =
                               { TA_Output_Real, "outFastD", TA_OUT_LINE };

static const TA_InputParameterInfo    *TA_STOCHRSI_Inputs[]    =
{
  &TA_DEF_UI_Input_Real,
  NULL
};

static const TA_OutputParameterInfo   *TA_STOCHRSI_Outputs[]   =
{
  &TA_DEF_UI_Output_Real_STOCHRSI_outFastK,
  &TA_DEF_UI_Output_Real_STOCHRSI_outFastD,
  NULL
};

static const TA_OptInputParameterInfo *TA_STOCHRSI_OptInputs[] =
{ &TA_DEF_UI_TimePeriod_14_MINIMUM2,
  &TA_DEF_UI_D_STOCHRSI_FastK_Period,
  &TA_DEF_UI_D_STOCHRSI_FastD_Period,
  &TA_DEF_UI_D_STOCHRSI_FastD_MAType,
  NULL
};

DEF_FUNCTION( STOCHRSI,
              TA_GroupId_MomentumIndicators,
              "Stochastic Relative Strength Index",
              TA_FUNC_FLG_STREAM
             );
/* STOCHRSI END */

/* SUB BEGIN */
static const TA_InputParameterInfo    *TA_SUB_Inputs[]    =
{
  &TA_DEF_UI_Input_Real0,
  &TA_DEF_UI_Input_Real1,
  NULL
};

static const TA_OutputParameterInfo   *TA_SUB_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_SUB_OptInputs[] =
{ NULL };

DEF_FUNCTION( SUB,
              TA_GroupId_MathOperators,
              "Vector Arithmetic Subtraction",
              TA_FUNC_FLG_STREAM
             );
/* SUB END */

/* SUM BEGIN */
static const TA_InputParameterInfo    *TA_SUM_Inputs[]    =
{
  &TA_DEF_UI_Input_Real,
  NULL
};

static const TA_OutputParameterInfo   *TA_SUM_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_SUM_OptInputs[] =
{ &TA_DEF_UI_TimePeriod_30_MINIMUM2,
  NULL
};

DEF_FUNCTION( SUM,
              TA_GroupId_MathOperators,
              "Summation",
              TA_FUNC_FLG_STREAM
             );
/* SUM END */

/* SUPERTREND BEGIN */
static const TA_OptInputParameterInfo TA_DEF_UI_D_SUPERTREND_TimePeriod =
{
   TA_OptInput_IntegerRange,
   "optInTimePeriod",
   0,

   "Time Period",
   (const void *)&TA_DEF_TimePeriod_Positive_Minimum2,
   10,
   "Time period for the Average True Range",

   NULL
};

static const TA_RealRange TA_DEF_SUPERTREND_Multiplier =
{
   0.0,
   TA_REAL_MAX,
   1,
   1.0,
   4.0,
   0.5
};

static const TA_OptInputParameterInfo TA_DEF_UI_D_SUPERTREND_Multiplier =
{
   TA_OptInput_RealRange,
   "optInMultiplier",
   0,

   "Multiplier",
   (const void *)&TA_DEF_SUPERTREND_Multiplier,
   3.0,
   "ATR multiplier for band width",

   NULL
};

const TA_OutputParameterInfo TA_DEF_UI_Output_Real_SUPERTREND_outSupertrend =
                               { TA_Output_Real, "outSupertrend", TA_OUT_LINE };

const TA_OutputParameterInfo TA_DEF_UI_Output_Integer_SUPERTREND_outTrend =
                               { TA_Output_Integer, "outTrend", TA_OUT_LINE };

static const TA_InputParameterInfo    *TA_SUPERTREND_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_HLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_SUPERTREND_Outputs[]   =
{
  &TA_DEF_UI_Output_Real_SUPERTREND_outSupertrend,
  &TA_DEF_UI_Output_Integer_SUPERTREND_outTrend,
  NULL
};

static const TA_OptInputParameterInfo *TA_SUPERTREND_OptInputs[] =
{ &TA_DEF_UI_D_SUPERTREND_TimePeriod,
  &TA_DEF_UI_D_SUPERTREND_Multiplier,
  NULL
};

DEF_FUNCTION( SUPERTREND,
              TA_GroupId_OverlapStudies,
              "SuperTrend",
              TA_FUNC_FLG_OVERLAP | TA_FUNC_FLG_STREAM | TA_FUNC_FLG_PATH_DEP
             );
/* SUPERTREND END */

/* SWAK_2PHP BEGIN */
static const TA_IntegerRange TA_DEF_SWAK_2PHP_TimePeriod =
{
   2,
   10000,
   5,
   200,
   1
};

static const TA_OptInputParameterInfo TA_DEF_UI_D_SWAK_2PHP_TimePeriod =
{
   TA_OptInput_IntegerRange,
   "optInTimePeriod",
   0,

   "Time Period",
   (const void *)&TA_DEF_SWAK_2PHP_TimePeriod,
   20,
   "Cutoff period",

   NULL
};

static const TA_InputParameterInfo    *TA_SWAK_2PHP_Inputs[]    =
{
  &TA_DEF_UI_Input_Real,
  NULL
};

static const TA_OutputParameterInfo   *TA_SWAK_2PHP_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_SWAK_2PHP_OptInputs[] =
{ &TA_DEF_UI_D_SWAK_2PHP_TimePeriod,
  NULL
};

DEF_FUNCTION( SWAK_2PHP,
              TA_GroupId_CycleIndicators,
              "Swiss Army Knife - Two-Pole High-Pass Filter",
              TA_FUNC_FLG_UNST_PER | TA_FUNC_FLG_STREAM
             );
/* SWAK_2PHP END */

/* SWAK_BP BEGIN */
static const TA_IntegerRange TA_DEF_SWAK_BP_TimePeriod =
{
   5,
   2000,
   5,
   200,
   1
};

static const TA_OptInputParameterInfo TA_DEF_UI_D_SWAK_BP_TimePeriod =
{
   TA_OptInput_IntegerRange,
   "optInTimePeriod",
   0,

   "Time Period",
   (const void *)&TA_DEF_SWAK_BP_TimePeriod,
   20,
   "Center period",

   NULL
};

static const TA_RealRange TA_DEF_SWAK_BP_Delta =
{
   0.05,
   0.5,
   2,
   0.05,
   0.5,
   0.05
};

static const TA_OptInputParameterInfo TA_DEF_UI_D_SWAK_BP_Delta =
{
   TA_OptInput_RealRange,
   "optInDelta",
   0,

   "Delta",
   (const void *)&TA_DEF_SWAK_BP_Delta,
   0.1,
   "Half-bandwidth as a fraction of the center period",

   NULL
};

static const TA_InputParameterInfo    *TA_SWAK_BP_Inputs[]    =
{
  &TA_DEF_UI_Input_Real,
  NULL
};

static const TA_OutputParameterInfo   *TA_SWAK_BP_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_SWAK_BP_OptInputs[] =
{ &TA_DEF_UI_D_SWAK_BP_TimePeriod,
  &TA_DEF_UI_D_SWAK_BP_Delta,
  NULL
};

DEF_FUNCTION( SWAK_BP,
              TA_GroupId_CycleIndicators,
              "Swiss Army Knife - Band-Pass Filter",
              TA_FUNC_FLG_UNST_PER | TA_FUNC_FLG_STREAM
             );
/* SWAK_BP END */

/* SWAK_BUTTER BEGIN */
static const TA_IntegerRange TA_DEF_SWAK_BUTTER_TimePeriod =
{
   2,
   10000,
   5,
   200,
   1
};

static const TA_OptInputParameterInfo TA_DEF_UI_D_SWAK_BUTTER_TimePeriod =
{
   TA_OptInput_IntegerRange,
   "optInTimePeriod",
   0,

   "Time Period",
   (const void *)&TA_DEF_SWAK_BUTTER_TimePeriod,
   20,
   "Cutoff period",

   NULL
};

static const TA_InputParameterInfo    *TA_SWAK_BUTTER_Inputs[]    =
{
  &TA_DEF_UI_Input_Real,
  NULL
};

static const TA_OutputParameterInfo   *TA_SWAK_BUTTER_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_SWAK_BUTTER_OptInputs[] =
{ &TA_DEF_UI_D_SWAK_BUTTER_TimePeriod,
  NULL
};

DEF_FUNCTION( SWAK_BUTTER,
              TA_GroupId_OverlapStudies,
              "Swiss Army Knife - Butterworth Filter",
              TA_FUNC_FLG_OVERLAP | TA_FUNC_FLG_UNST_PER | TA_FUNC_FLG_STREAM
             );
/* SWAK_BUTTER END */

/* SWAK_GAUSS BEGIN */
static const TA_IntegerRange TA_DEF_SWAK_GAUSS_TimePeriod =
{
   2,
   10000,
   5,
   200,
   1
};

static const TA_OptInputParameterInfo TA_DEF_UI_D_SWAK_GAUSS_TimePeriod =
{
   TA_OptInput_IntegerRange,
   "optInTimePeriod",
   0,

   "Time Period",
   (const void *)&TA_DEF_SWAK_GAUSS_TimePeriod,
   20,
   "Cutoff period",

   NULL
};

static const TA_InputParameterInfo    *TA_SWAK_GAUSS_Inputs[]    =
{
  &TA_DEF_UI_Input_Real,
  NULL
};

static const TA_OutputParameterInfo   *TA_SWAK_GAUSS_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_SWAK_GAUSS_OptInputs[] =
{ &TA_DEF_UI_D_SWAK_GAUSS_TimePeriod,
  NULL
};

DEF_FUNCTION( SWAK_GAUSS,
              TA_GroupId_OverlapStudies,
              "Swiss Army Knife - Gaussian Filter",
              TA_FUNC_FLG_OVERLAP | TA_FUNC_FLG_UNST_PER | TA_FUNC_FLG_STREAM
             );
/* SWAK_GAUSS END */

/* SWAK_HP BEGIN */
static const TA_OptInputParameterInfo TA_DEF_UI_D_SWAK_HP_TimePeriod =
{
   TA_OptInput_IntegerRange,
   "optInTimePeriod",
   0,

   "Time Period",
   (const void *)&TA_DEF_TimePeriod_Positive_Minimum5,
   20,
   "Cutoff period",

   NULL
};

static const TA_InputParameterInfo    *TA_SWAK_HP_Inputs[]    =
{
  &TA_DEF_UI_Input_Real,
  NULL
};

static const TA_OutputParameterInfo   *TA_SWAK_HP_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_SWAK_HP_OptInputs[] =
{ &TA_DEF_UI_D_SWAK_HP_TimePeriod,
  NULL
};

DEF_FUNCTION( SWAK_HP,
              TA_GroupId_CycleIndicators,
              "Swiss Army Knife - High-Pass Filter",
              TA_FUNC_FLG_UNST_PER | TA_FUNC_FLG_STREAM
             );
/* SWAK_HP END */

/****************************************************************************
 * Step 2 - Add your TA function to the table.
 *          Keep in alphabetical order. Must be NULL terminated.
 ****************************************************************************/
const TA_FuncDef *TA_DEF_TableS[] =
{
   ADD_TO_TABLE(SAR),
   ADD_TO_TABLE(SAREXT),
   ADD_TO_TABLE(SI),
   ADD_TO_TABLE(SIN),
   ADD_TO_TABLE(SINH),
   ADD_TO_TABLE(SMA),
   ADD_TO_TABLE(SMI),
   ADD_TO_TABLE(SQRT),
   ADD_TO_TABLE(SQZMOM),
   ADD_TO_TABLE(STC),
   ADD_TO_TABLE(STDDEV),
   ADD_TO_TABLE(STOCH),
   ADD_TO_TABLE(STOCHF),
   ADD_TO_TABLE(STOCHRSI),
   ADD_TO_TABLE(SUB),
   ADD_TO_TABLE(SUM),
   ADD_TO_TABLE(SUPERTREND),
   ADD_TO_TABLE(SWAK_2PHP),
   ADD_TO_TABLE(SWAK_BP),
   ADD_TO_TABLE(SWAK_BUTTER),
   ADD_TO_TABLE(SWAK_GAUSS),
   ADD_TO_TABLE(SWAK_HP),
   NULL
};


/* Do not modify the following line. */
const unsigned int TA_DEF_TableSSize =
              ((sizeof(TA_DEF_TableS)/sizeof(TA_FuncDef *))-1);

