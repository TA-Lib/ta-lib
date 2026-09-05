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
 * This file contains only TA functions starting with the letter 'R' *
 *********************************************************************/
#include <stddef.h>
#include "ta_abstract.h"
#include "ta_def_ui.h"

/* RMA BEGIN */
static const TA_InputParameterInfo    *TA_RMA_Inputs[]    =
{
  &TA_DEF_UI_Input_Real,
  NULL
};

static const TA_OutputParameterInfo   *TA_RMA_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_RMA_OptInputs[] =
{ &TA_DEF_UI_TimePeriod_30,
  NULL
};

DEF_FUNCTION( RMA,
              TA_GroupId_OverlapStudies,
              "Wilder's Smoothed Moving Average",
              TA_FUNC_FLG_OVERLAP | TA_FUNC_FLG_UNST_PER | TA_FUNC_FLG_STREAM | TA_FUNC_FLG_PERIOD1_IDENTITY
             );
/* RMA END */

/* ROC BEGIN */
static const TA_InputParameterInfo    *TA_ROC_Inputs[]    =
{
  &TA_DEF_UI_Input_Real,
  NULL
};

static const TA_OutputParameterInfo   *TA_ROC_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_ROC_OptInputs[] =
{ &TA_DEF_UI_TimePeriod_10,
  NULL
};

DEF_FUNCTION( ROC,
              TA_GroupId_MomentumIndicators,
              "Rate of change : ((price/prevPrice)-1)*100",
              TA_FUNC_FLG_STREAM
             );
/* ROC END */

/* ROCP BEGIN */
static const TA_InputParameterInfo    *TA_ROCP_Inputs[]    =
{
  &TA_DEF_UI_Input_Real,
  NULL
};

static const TA_OutputParameterInfo   *TA_ROCP_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_ROCP_OptInputs[] =
{ &TA_DEF_UI_TimePeriod_10,
  NULL
};

DEF_FUNCTION( ROCP,
              TA_GroupId_MomentumIndicators,
              "Rate of change Percentage: (price-prevPrice)/prevPrice",
              TA_FUNC_FLG_STREAM
             );
/* ROCP END */

/* ROCR BEGIN */
static const TA_InputParameterInfo    *TA_ROCR_Inputs[]    =
{
  &TA_DEF_UI_Input_Real,
  NULL
};

static const TA_OutputParameterInfo   *TA_ROCR_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_ROCR_OptInputs[] =
{ &TA_DEF_UI_TimePeriod_10,
  NULL
};

DEF_FUNCTION( ROCR,
              TA_GroupId_MomentumIndicators,
              "Rate of change ratio: (price/prevPrice)",
              TA_FUNC_FLG_STREAM
             );
/* ROCR END */

/* ROCR100 BEGIN */
static const TA_InputParameterInfo    *TA_ROCR100_Inputs[]    =
{
  &TA_DEF_UI_Input_Real,
  NULL
};

static const TA_OutputParameterInfo   *TA_ROCR100_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_ROCR100_OptInputs[] =
{ &TA_DEF_UI_TimePeriod_10,
  NULL
};

DEF_FUNCTION( ROCR100,
              TA_GroupId_MomentumIndicators,
              "Rate of change ratio 100 scale: (price/prevPrice)*100",
              TA_FUNC_FLG_STREAM
             );
/* ROCR100 END */

/* RSI BEGIN */
static const TA_InputParameterInfo    *TA_RSI_Inputs[]    =
{
  &TA_DEF_UI_Input_Real,
  NULL
};

static const TA_OutputParameterInfo   *TA_RSI_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_RSI_OptInputs[] =
{ &TA_DEF_UI_TimePeriod_14_MINIMUM2,
  NULL
};

DEF_FUNCTION( RSI,
              TA_GroupId_MomentumIndicators,
              "Relative Strength Index",
              TA_FUNC_FLG_UNST_PER | TA_FUNC_FLG_STREAM
             );
/* RSI END */

/* RVI BEGIN */
static const TA_IntegerRange TA_DEF_RVI_TimePeriod =
{
   1,
   100000,
   4,
   200,
   1
};

static const TA_OptInputParameterInfo TA_DEF_UI_D_RVI_TimePeriod =
{
   TA_OptInput_IntegerRange,
   "optInTimePeriod",
   0,

   "Time Period",
   (const void *)&TA_DEF_RVI_TimePeriod,
   14,
   "Time period of the Wilder smoothing applied to both legs",

   NULL
};

static const TA_OptInputParameterInfo TA_DEF_UI_D_RVI_StdDevPeriod =
{
   TA_OptInput_IntegerRange,
   "optInStdDevPeriod",
   0,

   "StdDev Period",
   (const void *)&TA_DEF_TimePeriod_Positive_Minimum2,
   10,
   "Time period of the standard deviation",

   NULL
};

static const TA_InputParameterInfo    *TA_RVI_Inputs[]    =
{
  &TA_DEF_UI_Input_Real,
  NULL
};

static const TA_OutputParameterInfo   *TA_RVI_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_RVI_OptInputs[] =
{ &TA_DEF_UI_D_RVI_TimePeriod,
  &TA_DEF_UI_D_RVI_StdDevPeriod,
  NULL
};

DEF_FUNCTION( RVI,
              TA_GroupId_VolatilityIndicators,
              "Relative Volatility Index",
              TA_FUNC_FLG_UNST_PER | TA_FUNC_FLG_STREAM
             );
/* RVI END */

/* RVOL BEGIN */
static const TA_OptInputParameterInfo TA_DEF_UI_D_RVOL_TimePeriod =
{
   TA_OptInput_IntegerRange,
   "optInTimePeriod",
   0,

   "Time Period",
   (const void *)&TA_DEF_TimePeriod_Positive,
   20,
   "Time period",

   NULL
};

static const TA_InputParameterInfo    *TA_RVOL_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_V,
  NULL
};

static const TA_OutputParameterInfo   *TA_RVOL_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_RVOL_OptInputs[] =
{ &TA_DEF_UI_D_RVOL_TimePeriod,
  NULL
};

DEF_FUNCTION( RVOL,
              TA_GroupId_VolumeIndicators,
              "Relative Volume",
              TA_FUNC_FLG_STREAM | TA_FUNC_FLG_NAN_INF_OUT
             );
/* RVOL END */

/****************************************************************************
 * Step 2 - Add your TA function to the table.
 *          Keep in alphabetical order. Must be NULL terminated.
 ****************************************************************************/
const TA_FuncDef *TA_DEF_TableR[] =
{
   ADD_TO_TABLE(RMA),
   ADD_TO_TABLE(ROC),
   ADD_TO_TABLE(ROCP),
   ADD_TO_TABLE(ROCR),
   ADD_TO_TABLE(ROCR100),
   ADD_TO_TABLE(RSI),
   ADD_TO_TABLE(RVI),
   ADD_TO_TABLE(RVOL),
   NULL
};


/* Do not modify the following line. */
const unsigned int TA_DEF_TableRSize =
              ((sizeof(TA_DEF_TableR)/sizeof(TA_FuncDef *))-1);

