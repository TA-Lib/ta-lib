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
 * This file contains only TA functions starting with the letter 'L' *
 *********************************************************************/
#include <stddef.h>
#include "ta_abstract.h"
#include "ta_def_ui.h"

/* LINEARREG BEGIN */
static const TA_InputParameterInfo    *TA_LINEARREG_Inputs[]    =
{
  &TA_DEF_UI_Input_Real,
  NULL
};

static const TA_OutputParameterInfo   *TA_LINEARREG_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_LINEARREG_OptInputs[] =
{ &TA_DEF_UI_TimePeriod_14_MINIMUM2,
  NULL
};

DEF_FUNCTION( LINEARREG,
              TA_GroupId_Statistic,
              "Linear Regression",
              "LinearReg",
              TA_FUNC_FLG_OVERLAP | TA_FUNC_FLG_STREAM
             );
/* LINEARREG END */

/* LINEARREG_ANGLE BEGIN */
static const TA_InputParameterInfo    *TA_LINEARREG_ANGLE_Inputs[]    =
{
  &TA_DEF_UI_Input_Real,
  NULL
};

static const TA_OutputParameterInfo   *TA_LINEARREG_ANGLE_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_LINEARREG_ANGLE_OptInputs[] =
{ &TA_DEF_UI_TimePeriod_14_MINIMUM2,
  NULL
};

DEF_FUNCTION( LINEARREG_ANGLE,
              TA_GroupId_Statistic,
              "Linear Regression Angle",
              "LinearRegAngle",
              TA_FUNC_FLG_STREAM
             );
/* LINEARREG_ANGLE END */

/* LINEARREG_INTERCEPT BEGIN */
static const TA_InputParameterInfo    *TA_LINEARREG_INTERCEPT_Inputs[]    =
{
  &TA_DEF_UI_Input_Real,
  NULL
};

static const TA_OutputParameterInfo   *TA_LINEARREG_INTERCEPT_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_LINEARREG_INTERCEPT_OptInputs[] =
{ &TA_DEF_UI_TimePeriod_14_MINIMUM2,
  NULL
};

DEF_FUNCTION( LINEARREG_INTERCEPT,
              TA_GroupId_Statistic,
              "Linear Regression Intercept",
              "LinearRegIntercept",
              TA_FUNC_FLG_OVERLAP | TA_FUNC_FLG_STREAM
             );
/* LINEARREG_INTERCEPT END */

/* LINEARREG_SLOPE BEGIN */
static const TA_InputParameterInfo    *TA_LINEARREG_SLOPE_Inputs[]    =
{
  &TA_DEF_UI_Input_Real,
  NULL
};

static const TA_OutputParameterInfo   *TA_LINEARREG_SLOPE_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_LINEARREG_SLOPE_OptInputs[] =
{ &TA_DEF_UI_TimePeriod_14_MINIMUM2,
  NULL
};

DEF_FUNCTION( LINEARREG_SLOPE,
              TA_GroupId_Statistic,
              "Linear Regression Slope",
              "LinearRegSlope",
              TA_FUNC_FLG_STREAM
             );
/* LINEARREG_SLOPE END */

/* LN BEGIN */
static const TA_InputParameterInfo    *TA_LN_Inputs[]    =
{
  &TA_DEF_UI_Input_Real,
  NULL
};

static const TA_OutputParameterInfo   *TA_LN_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_LN_OptInputs[] =
{ NULL };

DEF_FUNCTION( LN,
              TA_GroupId_MathTransform,
              "Vector Log Natural",
              "Ln",
              TA_FUNC_FLG_STREAM
             );
/* LN END */

/* LOG10 BEGIN */
static const TA_InputParameterInfo    *TA_LOG10_Inputs[]    =
{
  &TA_DEF_UI_Input_Real,
  NULL
};

static const TA_OutputParameterInfo   *TA_LOG10_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_LOG10_OptInputs[] =
{ NULL };

DEF_FUNCTION( LOG10,
              TA_GroupId_MathTransform,
              "Vector Log10",
              "Log10",
              TA_FUNC_FLG_STREAM
             );
/* LOG10 END */

/****************************************************************************
 * Step 2 - Add your TA function to the table.
 *          Keep in alphabetical order. Must be NULL terminated.
 ****************************************************************************/
const TA_FuncDef *TA_DEF_TableL[] =
{
   ADD_TO_TABLE(LINEARREG),
   ADD_TO_TABLE(LINEARREG_ANGLE),
   ADD_TO_TABLE(LINEARREG_INTERCEPT),
   ADD_TO_TABLE(LINEARREG_SLOPE),
   ADD_TO_TABLE(LN),
   ADD_TO_TABLE(LOG10),
   NULL
};


/* Do not modify the following line. */
const unsigned int TA_DEF_TableLSize =
              ((sizeof(TA_DEF_TableL)/sizeof(TA_FuncDef *))-1);

