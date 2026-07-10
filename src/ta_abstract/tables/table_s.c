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
              "Sar",
              TA_FUNC_FLG_OVERLAP | TA_FUNC_FLG_STREAM
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
              "SarExt",
              TA_FUNC_FLG_OVERLAP | TA_FUNC_FLG_STREAM
             );
/* SAREXT END */

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
              "Sin",
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
              "Sinh",
              TA_FUNC_FLG_STREAM
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
              "Sma",
              TA_FUNC_FLG_OVERLAP | TA_FUNC_FLG_STREAM
             );
/* SMA END */

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
              "Sqrt",
              TA_FUNC_FLG_STREAM
             );
/* SQRT END */

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
              "StdDev",
              0
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
              "Stoch",
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
              "StochF",
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
              "StochRsi",
              TA_FUNC_FLG_UNST_PER
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
              "Sub",
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
              "Sum",
              TA_FUNC_FLG_STREAM
             );
/* SUM END */

/****************************************************************************
 * Step 2 - Add your TA function to the table.
 *          Keep in alphabetical order. Must be NULL terminated.
 ****************************************************************************/
const TA_FuncDef *TA_DEF_TableS[] =
{
   ADD_TO_TABLE(SAR),
   ADD_TO_TABLE(SAREXT),
   ADD_TO_TABLE(SIN),
   ADD_TO_TABLE(SINH),
   ADD_TO_TABLE(SMA),
   ADD_TO_TABLE(SQRT),
   ADD_TO_TABLE(STDDEV),
   ADD_TO_TABLE(STOCH),
   ADD_TO_TABLE(STOCHF),
   ADD_TO_TABLE(STOCHRSI),
   ADD_TO_TABLE(SUB),
   ADD_TO_TABLE(SUM),
   NULL
};


/* Do not modify the following line. */
const unsigned int TA_DEF_TableSSize =
              ((sizeof(TA_DEF_TableS)/sizeof(TA_FuncDef *))-1);

