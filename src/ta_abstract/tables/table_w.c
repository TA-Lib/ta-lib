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
 * This file contains only TA functions starting with the letter 'W' *
 *********************************************************************/
#include <stddef.h>
#include "ta_abstract.h"
#include "ta_def_ui.h"

/* WAD BEGIN */
static const TA_InputParameterInfo    *TA_WAD_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_HLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_WAD_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_WAD_OptInputs[] =
{ NULL };

DEF_FUNCTION( WAD,
              TA_GroupId_MomentumIndicators,
              "Williams' Accumulation/Distribution",
              TA_FUNC_FLG_STREAM | TA_FUNC_FLG_PATH_DEP
             );
/* WAD END */

/* WAVETREND BEGIN */
static const TA_IntegerRange TA_DEF_WAVETREND_ChannelPeriod =
{
   2,
   100000,
   2,
   200,
   1
};

static const TA_OptInputParameterInfo TA_DEF_UI_D_WAVETREND_ChannelPeriod =
{
   TA_OptInput_IntegerRange,
   "optInChannelPeriod",
   0,

   "Channel Period",
   (const void *)&TA_DEF_WAVETREND_ChannelPeriod,
   10,
   "Period of the price channel, used by both the average and the deviation",

   NULL
};

static const TA_OptInputParameterInfo TA_DEF_UI_D_WAVETREND_AveragePeriod =
{
   TA_OptInput_IntegerRange,
   "optInAveragePeriod",
   0,

   "Average Period",
   (const void *)&TA_DEF_TimePeriod_Positive,
   21,
   "Smoothing for the oscillator line",

   NULL
};

static const TA_IntegerRange TA_DEF_WAVETREND_SignalPeriod =
{
   1,
   100000,
   1,
   50,
   1
};

static const TA_OptInputParameterInfo TA_DEF_UI_D_WAVETREND_SignalPeriod =
{
   TA_OptInput_IntegerRange,
   "optInSignalPeriod",
   0,

   "Signal Period",
   (const void *)&TA_DEF_WAVETREND_SignalPeriod,
   4,
   "Simple average of the oscillator line, making the signal line",

   NULL
};

const TA_OutputParameterInfo TA_DEF_UI_Output_Real_WAVETREND_outWT1 =
                               { TA_Output_Real, "outWT1", TA_OUT_LINE };

const TA_OutputParameterInfo TA_DEF_UI_Output_Real_WAVETREND_outWT2 =
                               { TA_Output_Real, "outWT2", TA_OUT_DASH_LINE };

static const TA_InputParameterInfo    *TA_WAVETREND_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_HLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_WAVETREND_Outputs[]   =
{
  &TA_DEF_UI_Output_Real_WAVETREND_outWT1,
  &TA_DEF_UI_Output_Real_WAVETREND_outWT2,
  NULL
};

static const TA_OptInputParameterInfo *TA_WAVETREND_OptInputs[] =
{ &TA_DEF_UI_D_WAVETREND_ChannelPeriod,
  &TA_DEF_UI_D_WAVETREND_AveragePeriod,
  &TA_DEF_UI_D_WAVETREND_SignalPeriod,
  NULL
};

DEF_FUNCTION( WAVETREND,
              TA_GroupId_MomentumIndicators,
              "WaveTrend Oscillator",
              TA_FUNC_FLG_STREAM
             );
/* WAVETREND END */

/* WCLPRICE BEGIN */
static const TA_InputParameterInfo    *TA_WCLPRICE_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_HLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_WCLPRICE_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_WCLPRICE_OptInputs[] =
{ NULL };

DEF_FUNCTION( WCLPRICE,
              TA_GroupId_PriceTransform,
              "Weighted Close Price",
              TA_FUNC_FLG_OVERLAP | TA_FUNC_FLG_STREAM
             );
/* WCLPRICE END */

/* WILLR BEGIN */
static const TA_InputParameterInfo    *TA_WILLR_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_HLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_WILLR_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_WILLR_OptInputs[] =
{ &TA_DEF_UI_TimePeriod_14_MINIMUM2,
  NULL
};

DEF_FUNCTION( WILLR,
              TA_GroupId_MomentumIndicators,
              "Williams' %R",
              TA_FUNC_FLG_STREAM
             );
/* WILLR END */

/* WMA BEGIN */
static const TA_InputParameterInfo    *TA_WMA_Inputs[]    =
{
  &TA_DEF_UI_Input_Real,
  NULL
};

static const TA_OutputParameterInfo   *TA_WMA_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_WMA_OptInputs[] =
{ &TA_DEF_UI_TimePeriod_30,
  NULL
};

DEF_FUNCTION( WMA,
              TA_GroupId_OverlapStudies,
              "Weighted Moving Average",
              TA_FUNC_FLG_OVERLAP | TA_FUNC_FLG_STREAM | TA_FUNC_FLG_PERIOD1_IDENTITY
             );
/* WMA END */

/****************************************************************************
 * Step 2 - Add your TA function to the table.
 *          Keep in alphabetical order. Must be NULL terminated.
 ****************************************************************************/
const TA_FuncDef *TA_DEF_TableW[] =
{
   ADD_TO_TABLE(WAD),
   ADD_TO_TABLE(WAVETREND),
   ADD_TO_TABLE(WCLPRICE),
   ADD_TO_TABLE(WILLR),
   ADD_TO_TABLE(WMA),
   NULL
};


/* Do not modify the following line. */
const unsigned int TA_DEF_TableWSize =
              ((sizeof(TA_DEF_TableW)/sizeof(TA_FuncDef *))-1);

