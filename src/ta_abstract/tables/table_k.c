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
 * This file contains only TA functions starting with the letter 'K' *
 *********************************************************************/
#include <stddef.h>
#include "ta_abstract.h"
#include "ta_def_ui.h"

/* KAMA BEGIN */
static const TA_InputParameterInfo    *TA_KAMA_Inputs[]    =
{
  &TA_DEF_UI_Input_Real,
  NULL
};

static const TA_OutputParameterInfo   *TA_KAMA_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_KAMA_OptInputs[] =
{ &TA_DEF_UI_TimePeriod_30,
  NULL
};

DEF_FUNCTION( KAMA,
              TA_GroupId_OverlapStudies,
              "Kaufman Adaptive Moving Average",
              TA_FUNC_FLG_OVERLAP | TA_FUNC_FLG_UNST_PER | TA_FUNC_FLG_STREAM | TA_FUNC_FLG_PERIOD1_IDENTITY
             );
/* KAMA END */

/* KC BEGIN */
static const TA_OptInputParameterInfo TA_DEF_UI_D_KC_TimePeriod =
{
   TA_OptInput_IntegerRange,
   "optInTimePeriod",
   0,

   "Time Period",
   (const void *)&TA_DEF_TimePeriod_Positive_Minimum2,
   20,
   "Time period for the typical price moving average",

   NULL
};

static const TA_OptInputParameterInfo TA_DEF_UI_D_KC_ATRPeriod =
{
   TA_OptInput_IntegerRange,
   "optInATRPeriod",
   0,

   "ATR Period",
   (const void *)&TA_DEF_TimePeriod_Positive,
   10,
   "Time period for the Average True Range",

   NULL
};

static const TA_RealRange TA_DEF_KC_NbDev =
{
   TA_REAL_MIN,
   TA_REAL_MAX,
   2,
   1.0,
   3.0,
   0.5
};

static const TA_OptInputParameterInfo TA_DEF_UI_D_KC_NbDev =
{
   TA_OptInput_RealRange,
   "optInNbDev",
   0,

   "Deviations",
   (const void *)&TA_DEF_KC_NbDev,
   2.0,
   "Multiplier applied to the Average True Range",

   NULL
};

const TA_OutputParameterInfo TA_DEF_UI_Output_Real_KC_UpperBand =
                               { TA_Output_Real, "outRealUpperBand", TA_OUT_UPPER_LIMIT };

const TA_OutputParameterInfo TA_DEF_UI_Output_Real_KC_MiddleBand =
                               { TA_Output_Real, "outRealMiddleBand", TA_OUT_LINE };

const TA_OutputParameterInfo TA_DEF_UI_Output_Real_KC_LowerBand =
                               { TA_Output_Real, "outRealLowerBand", TA_OUT_LOWER_LIMIT };

static const TA_InputParameterInfo    *TA_KC_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_HLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_KC_Outputs[]   =
{
  &TA_DEF_UI_Output_Real_KC_UpperBand,
  &TA_DEF_UI_Output_Real_KC_MiddleBand,
  &TA_DEF_UI_Output_Real_KC_LowerBand,
  NULL
};

static const TA_OptInputParameterInfo *TA_KC_OptInputs[] =
{ &TA_DEF_UI_D_KC_TimePeriod,
  &TA_DEF_UI_D_KC_ATRPeriod,
  &TA_DEF_UI_D_KC_NbDev,
  NULL
};

DEF_FUNCTION( KC,
              TA_GroupId_OverlapStudies,
              "Keltner Channels",
              TA_FUNC_FLG_OVERLAP | TA_FUNC_FLG_STREAM
             );
/* KC END */

/* KDJ BEGIN */
static const TA_OptInputParameterInfo TA_DEF_UI_D_KDJ_FastK_Period =
{
   TA_OptInput_IntegerRange,
   "optInFastK_Period",
   0,

   "Fast-K Period",
   (const void *)&TA_DEF_TimePeriod_Positive,
   9,
   "Time period for building the Fast-K line",

   NULL
};

static const TA_OptInputParameterInfo TA_DEF_UI_D_KDJ_SlowK_Period =
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

const TA_OptInputParameterInfo TA_DEF_UI_D_KDJ_SlowK_MAType =
{
   TA_OptInput_IntegerList,
   "optInSlowK_MAType",
   0,

   "Slow-K MA",
   (const void *)&TA_MA_TypeList,
   13,
   "Type of Moving Average for Slow-K",

   NULL
};

static const TA_OptInputParameterInfo TA_DEF_UI_D_KDJ_SlowD_Period =
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

const TA_OptInputParameterInfo TA_DEF_UI_D_KDJ_SlowD_MAType =
{
   TA_OptInput_IntegerList,
   "optInSlowD_MAType",
   0,

   "Slow-D MA",
   (const void *)&TA_MA_TypeList,
   13,
   "Type of Moving Average for Slow-D",

   NULL
};

const TA_OutputParameterInfo TA_DEF_UI_Output_Real_KDJ_outK =
                               { TA_Output_Real, "outK", TA_OUT_LINE };

const TA_OutputParameterInfo TA_DEF_UI_Output_Real_KDJ_outD =
                               { TA_Output_Real, "outD", TA_OUT_LINE };

const TA_OutputParameterInfo TA_DEF_UI_Output_Real_KDJ_outJ =
                               { TA_Output_Real, "outJ", TA_OUT_LINE };

static const TA_InputParameterInfo    *TA_KDJ_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_HLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_KDJ_Outputs[]   =
{
  &TA_DEF_UI_Output_Real_KDJ_outK,
  &TA_DEF_UI_Output_Real_KDJ_outD,
  &TA_DEF_UI_Output_Real_KDJ_outJ,
  NULL
};

static const TA_OptInputParameterInfo *TA_KDJ_OptInputs[] =
{ &TA_DEF_UI_D_KDJ_FastK_Period,
  &TA_DEF_UI_D_KDJ_SlowK_Period,
  &TA_DEF_UI_D_KDJ_SlowK_MAType,
  &TA_DEF_UI_D_KDJ_SlowD_Period,
  &TA_DEF_UI_D_KDJ_SlowD_MAType,
  NULL
};

DEF_FUNCTION( KDJ,
              TA_GroupId_MomentumIndicators,
              "KDJ Stochastic",
              TA_FUNC_FLG_STREAM
             );
/* KDJ END */

/****************************************************************************
 * Step 2 - Add your TA function to the table.
 *          Keep in alphabetical order. Must be NULL terminated.
 ****************************************************************************/
const TA_FuncDef *TA_DEF_TableK[] =
{
   ADD_TO_TABLE(KAMA),
   ADD_TO_TABLE(KC),
   ADD_TO_TABLE(KDJ),
   NULL
};


/* Do not modify the following line. */
const unsigned int TA_DEF_TableKSize =
              ((sizeof(TA_DEF_TableK)/sizeof(TA_FuncDef *))-1);

