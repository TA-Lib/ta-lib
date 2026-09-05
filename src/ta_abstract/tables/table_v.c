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
 * This file contains only TA functions starting with the letter 'V' *
 *********************************************************************/
#include <stddef.h>
#include "ta_abstract.h"
#include "ta_def_ui.h"

/* VAR BEGIN */
static const TA_InputParameterInfo    *TA_VAR_Inputs[]    =
{
  &TA_DEF_UI_Input_Real,
  NULL
};

static const TA_OutputParameterInfo   *TA_VAR_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_VAR_OptInputs[] =
{ &TA_DEF_UI_TimePeriod_5,
  &TA_DEF_UI_NbDeviation,
  NULL
};

DEF_FUNCTION( VAR,
              TA_GroupId_Statistic,
              "Variance",
              TA_FUNC_FLG_STREAM
             );
/* VAR END */

/* VHF BEGIN */
static const TA_IntegerRange TA_DEF_VHF_TimePeriod =
{
   2,
   100000,
   14,
   56,
   7
};

static const TA_OptInputParameterInfo TA_DEF_UI_D_VHF_TimePeriod =
{
   TA_OptInput_IntegerRange,
   "optInTimePeriod",
   0,

   "Time Period",
   (const void *)&TA_DEF_VHF_TimePeriod,
   28,
   "Time period",

   NULL
};

static const TA_InputParameterInfo    *TA_VHF_Inputs[]    =
{
  &TA_DEF_UI_Input_Real,
  NULL
};

static const TA_OutputParameterInfo   *TA_VHF_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_VHF_OptInputs[] =
{ &TA_DEF_UI_D_VHF_TimePeriod,
  NULL
};

DEF_FUNCTION( VHF,
              TA_GroupId_MomentumIndicators,
              "Vertical Horizontal Filter",
              TA_FUNC_FLG_STREAM
             );
/* VHF END */

/* VORTEX BEGIN */
static const TA_OptInputParameterInfo TA_DEF_UI_D_VORTEX_TimePeriod =
{
   TA_OptInput_IntegerRange,
   "optInTimePeriod",
   0,

   "Time Period",
   (const void *)&TA_DEF_TimePeriod_Positive,
   14,
   "Number of bars in the rolling sums",

   NULL
};

const TA_OutputParameterInfo TA_DEF_UI_Output_Real_VORTEX_outPlusVI =
                               { TA_Output_Real, "outPlusVI", TA_OUT_LINE };

const TA_OutputParameterInfo TA_DEF_UI_Output_Real_VORTEX_outMinusVI =
                               { TA_Output_Real, "outMinusVI", TA_OUT_LINE };

static const TA_InputParameterInfo    *TA_VORTEX_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_HLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_VORTEX_Outputs[]   =
{
  &TA_DEF_UI_Output_Real_VORTEX_outPlusVI,
  &TA_DEF_UI_Output_Real_VORTEX_outMinusVI,
  NULL
};

static const TA_OptInputParameterInfo *TA_VORTEX_OptInputs[] =
{ &TA_DEF_UI_D_VORTEX_TimePeriod,
  NULL
};

DEF_FUNCTION( VORTEX,
              TA_GroupId_MomentumIndicators,
              "Vortex Indicator",
              TA_FUNC_FLG_STREAM
             );
/* VORTEX END */

/* VWAP BEGIN */
static const TA_InputParameterInfo    *TA_VWAP_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_HLCV,
  NULL
};

static const TA_OutputParameterInfo   *TA_VWAP_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_VWAP_OptInputs[] =
{ NULL };

DEF_FUNCTION( VWAP,
              TA_GroupId_VolumeIndicators,
              "Volume Weighted Average Price",
              TA_FUNC_FLG_OVERLAP | TA_FUNC_FLG_STREAM | TA_FUNC_FLG_PATH_DEP
             );
/* VWAP END */

/* VWMA BEGIN */
static const TA_InputParameterInfo    *TA_VWMA_Inputs[]    =
{
  &TA_DEF_UI_Input_Real,
  &TA_DEF_UI_Input_Price_V,
  NULL
};

static const TA_OutputParameterInfo   *TA_VWMA_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_VWMA_OptInputs[] =
{ &TA_DEF_UI_TimePeriod_30,
  NULL
};

DEF_FUNCTION( VWMA,
              TA_GroupId_OverlapStudies,
              "Volume Weighted Moving Average",
              TA_FUNC_FLG_OVERLAP | TA_FUNC_FLG_STREAM | TA_FUNC_FLG_NAN_INF_OUT | TA_FUNC_FLG_PERIOD1_IDENTITY
             );
/* VWMA END */

/****************************************************************************
 * Step 2 - Add your TA function to the table.
 *          Keep in alphabetical order. Must be NULL terminated.
 ****************************************************************************/
const TA_FuncDef *TA_DEF_TableV[] =
{
   ADD_TO_TABLE(VAR),
   ADD_TO_TABLE(VHF),
   ADD_TO_TABLE(VORTEX),
   ADD_TO_TABLE(VWAP),
   ADD_TO_TABLE(VWMA),
   NULL
};


/* Do not modify the following line. */
const unsigned int TA_DEF_TableVSize =
              ((sizeof(TA_DEF_TableV)/sizeof(TA_FuncDef *))-1);

