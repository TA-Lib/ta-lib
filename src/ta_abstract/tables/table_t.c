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
 * This file contains only TA functions starting with the letter 'T' *
 *********************************************************************/
#include <stddef.h>
#include "ta_abstract.h"
#include "ta_def_ui.h"

/* T3 BEGIN */
static const TA_RealRange TA_DEF_T3_VFactor =
{
   0.0,
   1.0,
   2,
   0.01,
   1.0,
   0.05
};

static const TA_OptInputParameterInfo TA_DEF_UI_D_T3_VFactor =
{
   TA_OptInput_RealRange,
   "optInVFactor",
   0,

   "Volume Factor",
   (const void *)&TA_DEF_T3_VFactor,
   0.7,
   "Volume Factor",

   NULL
};

static const TA_InputParameterInfo    *TA_T3_Inputs[]    =
{
  &TA_DEF_UI_Input_Real,
  NULL
};

static const TA_OutputParameterInfo   *TA_T3_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_T3_OptInputs[] =
{ &TA_DEF_UI_TimePeriod_5,
  &TA_DEF_UI_D_T3_VFactor,
  NULL
};

DEF_FUNCTION( T3,
              TA_GroupId_OverlapStudies,
              "Triple Exponential Moving Average (T3)",
              "T3",
              TA_FUNC_FLG_OVERLAP | TA_FUNC_FLG_UNST_PER | TA_FUNC_FLG_STREAM
             );
/* T3 END */

/* TAN BEGIN */
static const TA_InputParameterInfo    *TA_TAN_Inputs[]    =
{
  &TA_DEF_UI_Input_Real,
  NULL
};

static const TA_OutputParameterInfo   *TA_TAN_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_TAN_OptInputs[] =
{ NULL };

DEF_FUNCTION( TAN,
              TA_GroupId_MathTransform,
              "Vector Trigonometric Tan",
              "Tan",
              TA_FUNC_FLG_STREAM
             );
/* TAN END */

/* TANH BEGIN */
static const TA_InputParameterInfo    *TA_TANH_Inputs[]    =
{
  &TA_DEF_UI_Input_Real,
  NULL
};

static const TA_OutputParameterInfo   *TA_TANH_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_TANH_OptInputs[] =
{ NULL };

DEF_FUNCTION( TANH,
              TA_GroupId_MathTransform,
              "Vector Trigonometric Tanh",
              "Tanh",
              TA_FUNC_FLG_STREAM
             );
/* TANH END */

/* TEMA BEGIN */
static const TA_InputParameterInfo    *TA_TEMA_Inputs[]    =
{
  &TA_DEF_UI_Input_Real,
  NULL
};

static const TA_OutputParameterInfo   *TA_TEMA_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_TEMA_OptInputs[] =
{ &TA_DEF_UI_TimePeriod_30,
  NULL
};

DEF_FUNCTION( TEMA,
              TA_GroupId_OverlapStudies,
              "Triple Exponential Moving Average",
              "Tema",
              TA_FUNC_FLG_OVERLAP | TA_FUNC_FLG_STREAM
             );
/* TEMA END */

/* TRANGE BEGIN */
static const TA_InputParameterInfo    *TA_TRANGE_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_HLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_TRANGE_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_TRANGE_OptInputs[] =
{ NULL };

DEF_FUNCTION( TRANGE,
              TA_GroupId_VolatilityIndicators,
              "True Range",
              "TrueRange",
              TA_FUNC_FLG_STREAM
             );
/* TRANGE END */

/* TRIMA BEGIN */
static const TA_InputParameterInfo    *TA_TRIMA_Inputs[]    =
{
  &TA_DEF_UI_Input_Real,
  NULL
};

static const TA_OutputParameterInfo   *TA_TRIMA_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_TRIMA_OptInputs[] =
{ &TA_DEF_UI_TimePeriod_30,
  NULL
};

DEF_FUNCTION( TRIMA,
              TA_GroupId_OverlapStudies,
              "Triangular Moving Average",
              "Trima",
              TA_FUNC_FLG_OVERLAP
             );
/* TRIMA END */

/* TRIX BEGIN */
static const TA_InputParameterInfo    *TA_TRIX_Inputs[]    =
{
  &TA_DEF_UI_Input_Real,
  NULL
};

static const TA_OutputParameterInfo   *TA_TRIX_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_TRIX_OptInputs[] =
{ &TA_DEF_UI_TimePeriod_30,
  NULL
};

DEF_FUNCTION( TRIX,
              TA_GroupId_MomentumIndicators,
              "1-day Rate-Of-Change (ROC) of a Triple Smooth EMA",
              "Trix",
              TA_FUNC_FLG_STREAM
             );
/* TRIX END */

/* TSF BEGIN */
static const TA_InputParameterInfo    *TA_TSF_Inputs[]    =
{
  &TA_DEF_UI_Input_Real,
  NULL
};

static const TA_OutputParameterInfo   *TA_TSF_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_TSF_OptInputs[] =
{ &TA_DEF_UI_TimePeriod_14_MINIMUM2,
  NULL
};

DEF_FUNCTION( TSF,
              TA_GroupId_Statistic,
              "Time Series Forecast",
              "Tsf",
              TA_FUNC_FLG_OVERLAP
             );
/* TSF END */

/* TYPPRICE BEGIN */
static const TA_InputParameterInfo    *TA_TYPPRICE_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_HLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_TYPPRICE_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_TYPPRICE_OptInputs[] =
{ NULL };

DEF_FUNCTION( TYPPRICE,
              TA_GroupId_PriceTransform,
              "Typical Price",
              "TypPrice",
              TA_FUNC_FLG_OVERLAP | TA_FUNC_FLG_STREAM
             );
/* TYPPRICE END */

/****************************************************************************
 * Step 2 - Add your TA function to the table.
 *          Keep in alphabetical order. Must be NULL terminated.
 ****************************************************************************/
const TA_FuncDef *TA_DEF_TableT[] =
{
   ADD_TO_TABLE(T3),
   ADD_TO_TABLE(TAN),
   ADD_TO_TABLE(TANH),
   ADD_TO_TABLE(TEMA),
   ADD_TO_TABLE(TRANGE),
   ADD_TO_TABLE(TRIMA),
   ADD_TO_TABLE(TRIX),
   ADD_TO_TABLE(TSF),
   ADD_TO_TABLE(TYPPRICE),
   NULL
};


/* Do not modify the following line. */
const unsigned int TA_DEF_TableTSize =
              ((sizeof(TA_DEF_TableT)/sizeof(TA_FuncDef *))-1);

