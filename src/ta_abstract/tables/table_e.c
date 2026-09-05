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
 * This file contains only TA functions starting with the letter 'E' *
 *********************************************************************/
#include <stddef.h>
#include "ta_abstract.h"
#include "ta_def_ui.h"

/* EFI BEGIN */
static const TA_OptInputParameterInfo TA_DEF_UI_D_EFI_TimePeriod =
{
   TA_OptInput_IntegerRange,
   "optInTimePeriod",
   0,

   "Time Period",
   (const void *)&TA_DEF_TimePeriod_Positive,
   13,
   "Time period",

   NULL
};

static const TA_InputParameterInfo    *TA_EFI_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_CV,
  NULL
};

static const TA_OutputParameterInfo   *TA_EFI_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_EFI_OptInputs[] =
{ &TA_DEF_UI_D_EFI_TimePeriod,
  NULL
};

DEF_FUNCTION( EFI,
              TA_GroupId_VolumeIndicators,
              "Elder's Force Index",
              TA_FUNC_FLG_STREAM
             );
/* EFI END */

/* EMA BEGIN */
static const TA_InputParameterInfo    *TA_EMA_Inputs[]    =
{
  &TA_DEF_UI_Input_Real,
  NULL
};

static const TA_OutputParameterInfo   *TA_EMA_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_EMA_OptInputs[] =
{ &TA_DEF_UI_TimePeriod_30,
  NULL
};

DEF_FUNCTION( EMA,
              TA_GroupId_OverlapStudies,
              "Exponential Moving Average",
              TA_FUNC_FLG_OVERLAP | TA_FUNC_FLG_UNST_PER | TA_FUNC_FLG_STREAM | TA_FUNC_FLG_PERIOD1_IDENTITY
             );
/* EMA END */

/* ERI BEGIN */
static const TA_OptInputParameterInfo TA_DEF_UI_D_ERI_TimePeriod =
{
   TA_OptInput_IntegerRange,
   "optInTimePeriod",
   0,

   "Time Period",
   (const void *)&TA_DEF_TimePeriod_Positive,
   13,
   "Number of bars in the EMA of close",

   NULL
};

const TA_OutputParameterInfo TA_DEF_UI_Output_Real_ERI_outBullPower =
                               { TA_Output_Real, "outBullPower", TA_OUT_LINE };

const TA_OutputParameterInfo TA_DEF_UI_Output_Real_ERI_outBearPower =
                               { TA_Output_Real, "outBearPower", TA_OUT_LINE };

static const TA_InputParameterInfo    *TA_ERI_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_HLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_ERI_Outputs[]   =
{
  &TA_DEF_UI_Output_Real_ERI_outBullPower,
  &TA_DEF_UI_Output_Real_ERI_outBearPower,
  NULL
};

static const TA_OptInputParameterInfo *TA_ERI_OptInputs[] =
{ &TA_DEF_UI_D_ERI_TimePeriod,
  NULL
};

DEF_FUNCTION( ERI,
              TA_GroupId_MomentumIndicators,
              "Elder Ray Index (Bull Power / Bear Power)",
              TA_FUNC_FLG_STREAM
             );
/* ERI END */

/* EXP BEGIN */
static const TA_InputParameterInfo    *TA_EXP_Inputs[]    =
{
  &TA_DEF_UI_Input_Real,
  NULL
};

static const TA_OutputParameterInfo   *TA_EXP_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_EXP_OptInputs[] =
{ NULL };

DEF_FUNCTION( EXP,
              TA_GroupId_MathTransform,
              "Vector Arithmetic Exp",
              TA_FUNC_FLG_STREAM
             );
/* EXP END */

/****************************************************************************
 * Step 2 - Add your TA function to the table.
 *          Keep in alphabetical order. Must be NULL terminated.
 ****************************************************************************/
const TA_FuncDef *TA_DEF_TableE[] =
{
   ADD_TO_TABLE(EFI),
   ADD_TO_TABLE(EMA),
   ADD_TO_TABLE(ERI),
   ADD_TO_TABLE(EXP),
   NULL
};


/* Do not modify the following line. */
const unsigned int TA_DEF_TableESize =
              ((sizeof(TA_DEF_TableE)/sizeof(TA_FuncDef *))-1);

