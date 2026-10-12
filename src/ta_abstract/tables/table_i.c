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
 * This file contains only TA functions starting with the letter 'I' *
 *********************************************************************/
#include <stddef.h>
#include "ta_abstract.h"
#include "ta_def_ui.h"

/* IBS BEGIN */
static const TA_InputParameterInfo    *TA_IBS_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_HLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_IBS_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_IBS_OptInputs[] =
{ NULL };

DEF_FUNCTION( IBS,
              TA_GroupId_MomentumIndicators,
              "Internal Bar Strength",
              TA_FUNC_FLG_STREAM
             );
/* IBS END */

/* ICHIMOKU BEGIN */
static const TA_OptInputParameterInfo TA_DEF_UI_D_ICHIMOKU_TenkanPeriod =
{
   TA_OptInput_IntegerRange,
   "optInTenkanPeriod",
   0,

   "Tenkan Period",
   (const void *)&TA_DEF_TimePeriod_Positive_Minimum2,
   9,
   "Period of the conversion line",

   NULL
};

static const TA_OptInputParameterInfo TA_DEF_UI_D_ICHIMOKU_KijunPeriod =
{
   TA_OptInput_IntegerRange,
   "optInKijunPeriod",
   0,

   "Kijun Period",
   (const void *)&TA_DEF_TimePeriod_Positive_Minimum2,
   26,
   "Period of the base line, and the forward shift of the two spans",

   NULL
};

static const TA_OptInputParameterInfo TA_DEF_UI_D_ICHIMOKU_SenkouBPeriod =
{
   TA_OptInput_IntegerRange,
   "optInSenkouBPeriod",
   0,

   "Senkou B Period",
   (const void *)&TA_DEF_TimePeriod_Positive_Minimum2,
   52,
   "Period of the second leading span",

   NULL
};

const TA_OutputParameterInfo TA_DEF_UI_Output_Real_ICHIMOKU_outTenkanSen =
                               { TA_Output_Real, "outTenkanSen", TA_OUT_LINE };

const TA_OutputParameterInfo TA_DEF_UI_Output_Real_ICHIMOKU_outKijunSen =
                               { TA_Output_Real, "outKijunSen", TA_OUT_LINE };

const TA_OutputParameterInfo TA_DEF_UI_Output_Real_ICHIMOKU_outSenkouSpanA =
                               { TA_Output_Real, "outSenkouSpanA", TA_OUT_LINE | TA_OUT_DISPLAY_SHIFT };

const TA_OutputParameterInfo TA_DEF_UI_Output_Real_ICHIMOKU_outSenkouSpanB =
                               { TA_Output_Real, "outSenkouSpanB", TA_OUT_LINE | TA_OUT_DISPLAY_SHIFT };

static const TA_InputParameterInfo    *TA_ICHIMOKU_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_HL,
  NULL
};

static const TA_OutputParameterInfo   *TA_ICHIMOKU_Outputs[]   =
{
  &TA_DEF_UI_Output_Real_ICHIMOKU_outTenkanSen,
  &TA_DEF_UI_Output_Real_ICHIMOKU_outKijunSen,
  &TA_DEF_UI_Output_Real_ICHIMOKU_outSenkouSpanA,
  &TA_DEF_UI_Output_Real_ICHIMOKU_outSenkouSpanB,
  NULL
};

static const TA_OptInputParameterInfo *TA_ICHIMOKU_OptInputs[] =
{ &TA_DEF_UI_D_ICHIMOKU_TenkanPeriod,
  &TA_DEF_UI_D_ICHIMOKU_KijunPeriod,
  &TA_DEF_UI_D_ICHIMOKU_SenkouBPeriod,
  NULL
};

DEF_FUNCTION( ICHIMOKU,
              TA_GroupId_OverlapStudies,
              "Ichimoku Kinko Hyo",
              TA_FUNC_FLG_OVERLAP | TA_FUNC_FLG_STREAM | TA_FUNC_FLG_DISPLAY_SHIFT
             );
/* ICHIMOKU END */

/* IMI BEGIN */
static const TA_InputParameterInfo    *TA_IMI_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_OC,
  NULL
};

static const TA_OutputParameterInfo   *TA_IMI_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_IMI_OptInputs[] =
{ &TA_DEF_UI_TimePeriod_14_MINIMUM2,
  NULL
};

DEF_FUNCTION( IMI,
              TA_GroupId_MomentumIndicators,
              "Intraday Momentum Index",
              TA_FUNC_FLG_STREAM
             );
/* IMI END */

/****************************************************************************
 * Step 2 - Add your TA function to the table.
 *          Keep in alphabetical order. Must be NULL terminated.
 ****************************************************************************/
const TA_FuncDef *TA_DEF_TableI[] =
{
   ADD_TO_TABLE(IBS),
   ADD_TO_TABLE(ICHIMOKU),
   ADD_TO_TABLE(IMI),
   NULL
};


/* Do not modify the following line. */
const unsigned int TA_DEF_TableISize =
              ((sizeof(TA_DEF_TableI)/sizeof(TA_FuncDef *))-1);

