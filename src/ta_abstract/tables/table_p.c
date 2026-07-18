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
 * This file contains only TA functions starting with the letter 'P' *
 *********************************************************************/
#include <stddef.h>
#include "ta_abstract.h"
#include "ta_def_ui.h"

/* PLUS_DI BEGIN */
static const TA_InputParameterInfo    *TA_PLUS_DI_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_HLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_PLUS_DI_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_PLUS_DI_OptInputs[] =
{ &TA_DEF_UI_TimePeriod_14,
  NULL
};

DEF_FUNCTION( PLUS_DI,
              TA_GroupId_MomentumIndicators,
              "Plus Directional Indicator",
              "PlusDI",
              TA_FUNC_FLG_UNST_PER | TA_FUNC_FLG_STREAM
             );
/* PLUS_DI END */

/* PLUS_DM BEGIN */
static const TA_InputParameterInfo    *TA_PLUS_DM_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_HL,
  NULL
};

static const TA_OutputParameterInfo   *TA_PLUS_DM_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_PLUS_DM_OptInputs[] =
{ &TA_DEF_UI_TimePeriod_14,
  NULL
};

DEF_FUNCTION( PLUS_DM,
              TA_GroupId_MomentumIndicators,
              "Plus Directional Movement",
              "PlusDM",
              TA_FUNC_FLG_UNST_PER | TA_FUNC_FLG_STREAM
             );
/* PLUS_DM END */

/* PPO BEGIN */
const TA_OptInputParameterInfo TA_DEF_UI_D_PPO_MAType =
{
   TA_OptInput_IntegerList,
   "optInMAType",
   0,

   "MA Type",
   (const void *)&TA_MA_TypeList,
   1,
   "Type of Moving Average",

   NULL
};

static const TA_InputParameterInfo    *TA_PPO_Inputs[]    =
{
  &TA_DEF_UI_Input_Real,
  NULL
};

static const TA_OutputParameterInfo   *TA_PPO_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_PPO_OptInputs[] =
{ &TA_DEF_UI_Fast_Period,
  &TA_DEF_UI_Slow_Period,
  &TA_DEF_UI_D_PPO_MAType,
  NULL
};

DEF_FUNCTION( PPO,
              TA_GroupId_MomentumIndicators,
              "Percentage Price Oscillator",
              "Ppo",
              TA_FUNC_FLG_STREAM
             );
/* PPO END */

/* PVI BEGIN */
static const TA_InputParameterInfo    *TA_PVI_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_CV,
  NULL
};

static const TA_OutputParameterInfo   *TA_PVI_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_PVI_OptInputs[] =
{ NULL };

DEF_FUNCTION( PVI,
              TA_GroupId_VolumeIndicators,
              "Positive Volume Index",
              "Pvi",
              TA_FUNC_FLG_STREAM
             );
/* PVI END */

/* PVO BEGIN */
const TA_OptInputParameterInfo TA_DEF_UI_D_PVO_MAType =
{
   TA_OptInput_IntegerList,
   "optInMAType",
   0,

   "MA Type",
   (const void *)&TA_MA_TypeList,
   1,
   "Type of Moving Average",

   NULL
};

static const TA_InputParameterInfo    *TA_PVO_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_V,
  NULL
};

static const TA_OutputParameterInfo   *TA_PVO_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_PVO_OptInputs[] =
{ &TA_DEF_UI_Fast_Period,
  &TA_DEF_UI_Slow_Period,
  &TA_DEF_UI_D_PVO_MAType,
  NULL
};

DEF_FUNCTION( PVO,
              TA_GroupId_VolumeIndicators,
              "Percentage Volume Oscillator",
              "Pvo",
              TA_FUNC_FLG_STREAM
             );
/* PVO END */

/****************************************************************************
 * Step 2 - Add your TA function to the table.
 *          Keep in alphabetical order. Must be NULL terminated.
 ****************************************************************************/
const TA_FuncDef *TA_DEF_TableP[] =
{
   ADD_TO_TABLE(PLUS_DI),
   ADD_TO_TABLE(PLUS_DM),
   ADD_TO_TABLE(PPO),
   ADD_TO_TABLE(PVI),
   ADD_TO_TABLE(PVO),
   NULL
};


/* Do not modify the following line. */
const unsigned int TA_DEF_TablePSize =
              ((sizeof(TA_DEF_TableP)/sizeof(TA_FuncDef *))-1);

