/* TA-LIB Copyright (c) 1999-2004, Mario Fortier
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
 * This file contains only TA functions starting with the letter 'C' *
 *********************************************************************/
#include <stddef.h>
#include "ta_abstract.h"
#include "ta_def_ui.h"

/* Follow the 3 steps defined below for adding a new TA Function to this
 * file.
 */

/****************************************************************************
 * Step 1 - Define here the interface to your TA functions with
 *          the macro DEF_FUNCTION.
 *
 ****************************************************************************/

/* CCI BEGIN */
static const TA_InputParameterInfo    *TA_CCI_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_HLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_CCI_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_CCI_OptInputs[] =
{ &TA_DEF_UI_TimePeriod_14_MINIMUM2,
  NULL
};

DEF_FUNCTION( CCI,                           /* name */
              TA_GroupId_MomentumIndicators, /* groupId */
              "Commodity Channel Index",     /* hint */
              NULL,                          /* helpFile */
              0,                             /* flags */
              NULL                           /* analysis function */
             );

/* CCI END */

/* CDL2CROWS BEGIN */
static const TA_InputParameterInfo    *TA_CDL2CROWS_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_OHLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_CDL2CROWS_Outputs[]   =
{
  &TA_DEF_UI_Output_Integer,
  NULL
};

static const TA_OptInputParameterInfo *TA_CDL2CROWS_OptInputs[] =
{ 
  NULL
};

DEF_FUNCTION( CDL2CROWS,                      /* name */
              TA_GroupId_PatternRecognition,  /* groupId */
              "Two Crows",                    /* hint */
              NULL,                           /* helpFile */
              TA_FUNC_FLG_CANDLESTICK,        /* flags */
              NULL                            /* analysis function */
             );

/* CDL2CROWS END */

/* CDL3BLACKCROWS BEGIN */
static const TA_InputParameterInfo    *TA_CDL3BLACKCROWS_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_OHLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_CDL3BLACKCROWS_Outputs[]   =
{
  &TA_DEF_UI_Output_Integer,
  NULL
};

static const TA_OptInputParameterInfo *TA_CDL3BLACKCROWS_OptInputs[] =
{ 
  NULL
};

DEF_FUNCTION( CDL3BLACKCROWS,                 /* name */
              TA_GroupId_PatternRecognition,  /* groupId */
              "Three Black Crows",            /* hint */
              NULL,                           /* helpFile */
              TA_FUNC_FLG_CANDLESTICK,        /* flags */
              NULL                            /* analysis function */
             );

/* CDL3BLACKCROWS END */

/* CDL3INSIDE BEGIN */
static const TA_InputParameterInfo    *TA_CDL3INSIDE_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_OHLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_CDL3INSIDE_Outputs[]   =
{
  &TA_DEF_UI_Output_Integer,
  NULL
};

static const TA_OptInputParameterInfo *TA_CDL3INSIDE_OptInputs[] =
{ 
  NULL
};

DEF_FUNCTION( CDL3INSIDE,                     /* name */
              TA_GroupId_PatternRecognition,  /* groupId */
              "Three Inside Up/Down",         /* hint */
              NULL,                           /* helpFile */
              TA_FUNC_FLG_CANDLESTICK,        /* flags */
              NULL                            /* analysis function */
             );

/* CDL3INSIDE END */

/* CDL3LINESTRIKE BEGIN */
static const TA_InputParameterInfo    *TA_CDL3LINESTRIKE_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_OHLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_CDL3LINESTRIKE_Outputs[]   =
{
  &TA_DEF_UI_Output_Integer,
  NULL
};

static const TA_OptInputParameterInfo *TA_CDL3LINESTRIKE_OptInputs[] =
{ 
  NULL
};

DEF_FUNCTION( CDL3LINESTRIKE,                 /* name */
              TA_GroupId_PatternRecognition,  /* groupId */
              "Three-Line Strike ",           /* hint */
              NULL,                           /* helpFile */
              TA_FUNC_FLG_CANDLESTICK,        /* flags */
              NULL                            /* analysis function */
             );

/* CDL3LINESTRIKE END */

/* CDL3OUTSIDE BEGIN */
static const TA_InputParameterInfo    *TA_CDL3OUTSIDE_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_OHLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_CDL3OUTSIDE_Outputs[]   =
{
  &TA_DEF_UI_Output_Integer,
  NULL
};

static const TA_OptInputParameterInfo *TA_CDL3OUTSIDE_OptInputs[] =
{ 
  NULL
};

DEF_FUNCTION( CDL3OUTSIDE,                    /* name */
              TA_GroupId_PatternRecognition,  /* groupId */
              "Three Outside Up/Down",        /* hint */
              NULL,                           /* helpFile */
              TA_FUNC_FLG_CANDLESTICK,        /* flags */
              NULL                            /* analysis function */
             );

/* CDL3OUTSIDE END */

/* CDL3WHITESOLDIERS BEGIN */
static const TA_InputParameterInfo    *TA_CDL3WHITESOLDIERS_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_OHLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_CDL3WHITESOLDIERS_Outputs[]   =
{
  &TA_DEF_UI_Output_Integer,
  NULL
};

static const TA_OptInputParameterInfo *TA_CDL3WHITESOLDIERS_OptInputs[] =
{ 
  NULL
};

DEF_FUNCTION( CDL3WHITESOLDIERS,              /* name */
              TA_GroupId_PatternRecognition,  /* groupId */
              "Three Advancing White Soldiers", /* hint */
              NULL,                           /* helpFile */
              TA_FUNC_FLG_CANDLESTICK,        /* flags */
              NULL                            /* analysis function */
             );

/* CDL3WHITESOLDIERS END */

/* CDLABANDONEDBABY BEGIN */
static const TA_InputParameterInfo    *TA_CDLABANDONEDBABY_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_OHLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_CDLABANDONEDBABY_Outputs[]   =
{
  &TA_DEF_UI_Output_Integer,
  NULL
};

static const TA_OptInputParameterInfo *TA_CDLABANDONEDBABY_OptInputs[] =
{ 
  &TA_DEF_UI_Penetration_30,
  NULL
};

DEF_FUNCTION( CDLABANDONEDBABY,               /* name */
              TA_GroupId_PatternRecognition,  /* groupId */
              "Abandoned Baby",               /* hint */
              NULL,                           /* helpFile */
              TA_FUNC_FLG_CANDLESTICK,        /* flags */
              NULL                            /* analysis function */
             );

/* CDLABANDONEDBABY END */

/* CDLADVANCEBLOCK BEGIN */
static const TA_InputParameterInfo    *TA_CDLADVANCEBLOCK_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_OHLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_CDLADVANCEBLOCK_Outputs[]   =
{
  &TA_DEF_UI_Output_Integer,
  NULL
};

static const TA_OptInputParameterInfo *TA_CDLADVANCEBLOCK_OptInputs[] =
{ 
  NULL
};

DEF_FUNCTION( CDLADVANCEBLOCK,                /* name */
              TA_GroupId_PatternRecognition,  /* groupId */
              "Advance Block",                /* hint */
              NULL,                           /* helpFile */
              TA_FUNC_FLG_CANDLESTICK,        /* flags */
              NULL                            /* analysis function */
             );

/* CDLADVANCEBLOCK END */

/* CDLBELTHOLD BEGIN */
static const TA_InputParameterInfo    *TA_CDLBELTHOLD_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_OHLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_CDLBELTHOLD_Outputs[]   =
{
  &TA_DEF_UI_Output_Integer,
  NULL
};

static const TA_OptInputParameterInfo *TA_CDLBELTHOLD_OptInputs[] =
{ 
  NULL
};

DEF_FUNCTION( CDLBELTHOLD,                    /* name */
              TA_GroupId_PatternRecognition,  /* groupId */
              "Belt-hold",                    /* hint */
              NULL,                           /* helpFile */
              TA_FUNC_FLG_CANDLESTICK,        /* flags */
              NULL                            /* analysis function */
             );

/* CDLBELTHOLD END */

/* CDLBREAKAWAY BEGIN */
static const TA_InputParameterInfo    *TA_CDLBREAKAWAY_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_OHLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_CDLBREAKAWAY_Outputs[]   =
{
  &TA_DEF_UI_Output_Integer,
  NULL
};

static const TA_OptInputParameterInfo *TA_CDLBREAKAWAY_OptInputs[] =
{ 
  NULL
};

DEF_FUNCTION( CDLBREAKAWAY,                   /* name */
              TA_GroupId_PatternRecognition,  /* groupId */
              "Breakaway",                    /* hint */
              NULL,                           /* helpFile */
              TA_FUNC_FLG_CANDLESTICK,        /* flags */
              NULL                            /* analysis function */
             );

/* CDLBREAKAWAY END */

/* CDLCOUNTERATTACK BEGIN */
static const TA_InputParameterInfo    *TA_CDLCOUNTERATTACK_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_OHLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_CDLCOUNTERATTACK_Outputs[]   =
{
  &TA_DEF_UI_Output_Integer,
  NULL
};

static const TA_OptInputParameterInfo *TA_CDLCOUNTERATTACK_OptInputs[] =
{ 
  NULL
};

DEF_FUNCTION( CDLCOUNTERATTACK,               /* name */
              TA_GroupId_PatternRecognition,  /* groupId */
              "Counterattack",                /* hint */
              NULL,                           /* helpFile */
              TA_FUNC_FLG_CANDLESTICK,        /* flags */
              NULL                            /* analysis function */
             );

/* CDLCOUNTERATTACK END */

/* CDLDARKCLOUDCOVER BEGIN */
static const TA_InputParameterInfo    *TA_CDLDARKCLOUDCOVER_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_OHLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_CDLDARKCLOUDCOVER_Outputs[]   =
{
  &TA_DEF_UI_Output_Integer,
  NULL
};

static const TA_OptInputParameterInfo *TA_CDLDARKCLOUDCOVER_OptInputs[] =
{ 
  &TA_DEF_UI_Penetration_50,
  NULL
};

DEF_FUNCTION( CDLDARKCLOUDCOVER,              /* name */
              TA_GroupId_PatternRecognition,  /* groupId */
              "Dark Cloud Cover",             /* hint */
              NULL,                           /* helpFile */
              TA_FUNC_FLG_CANDLESTICK,        /* flags */
              NULL                            /* analysis function */
             );

/* CDLDARKCLOUDCOVER END */

/* CDLDOJI BEGIN */
static const TA_InputParameterInfo    *TA_CDLDOJI_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_OHLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_CDLDOJI_Outputs[]   =
{
  &TA_DEF_UI_Output_Integer,
  NULL
};

static const TA_OptInputParameterInfo *TA_CDLDOJI_OptInputs[] =
{ 
  NULL
};

DEF_FUNCTION( CDLDOJI,                        /* name */
              TA_GroupId_PatternRecognition,  /* groupId */
              "Doji",                         /* hint */
              NULL,                           /* helpFile */
              TA_FUNC_FLG_CANDLESTICK,        /* flags */
              NULL                            /* analysis function */
             );

/* CDLDOJI END */

/* CDLDOJISTAR BEGIN */
static const TA_InputParameterInfo    *TA_CDLDOJISTAR_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_OHLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_CDLDOJISTAR_Outputs[]   =
{
  &TA_DEF_UI_Output_Integer,
  NULL
};

static const TA_OptInputParameterInfo *TA_CDLDOJISTAR_OptInputs[] =
{ 
  NULL
};

DEF_FUNCTION( CDLDOJISTAR,                    /* name */
              TA_GroupId_PatternRecognition,  /* groupId */
              "Doji Star",                    /* hint */
              NULL,                           /* helpFile */
              TA_FUNC_FLG_CANDLESTICK,        /* flags */
              NULL                            /* analysis function */
             );

/* CDLDOJISTAR END */

/* CDLDRAGONFLYDOJI BEGIN */
static const TA_InputParameterInfo    *TA_CDLDRAGONFLYDOJI_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_OHLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_CDLDRAGONFLYDOJI_Outputs[]   =
{
  &TA_DEF_UI_Output_Integer,
  NULL
};

static const TA_OptInputParameterInfo *TA_CDLDRAGONFLYDOJI_OptInputs[] =
{ 
  NULL
};

DEF_FUNCTION( CDLDRAGONFLYDOJI,               /* name */
              TA_GroupId_PatternRecognition,  /* groupId */
              "Dragonfly Doji",               /* hint */
              NULL,                           /* helpFile */
              TA_FUNC_FLG_CANDLESTICK,        /* flags */
              NULL                            /* analysis function */
             );

/* CDLDRAGONFLYDOJI END */

/* CDLENGULFING BEGIN */
static const TA_InputParameterInfo    *TA_CDLENGULFING_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_OHLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_CDLENGULFING_Outputs[]   =
{
  &TA_DEF_UI_Output_Integer,
  NULL
};

static const TA_OptInputParameterInfo *TA_CDLENGULFING_OptInputs[] =
{ 
  NULL
};

DEF_FUNCTION( CDLENGULFING,                   /* name */
              TA_GroupId_PatternRecognition,  /* groupId */
              "Engulfing Pattern",            /* hint */
              NULL,                           /* helpFile */
              TA_FUNC_FLG_CANDLESTICK,        /* flags */
              NULL                            /* analysis function */
             );

/* CDLENGULFING END */

/* CDLEVENINGDOJISTAR BEGIN */
static const TA_InputParameterInfo    *TA_CDLEVENINGDOJISTAR_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_OHLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_CDLEVENINGDOJISTAR_Outputs[]   =
{
  &TA_DEF_UI_Output_Integer,
  NULL
};

static const TA_OptInputParameterInfo *TA_CDLEVENINGDOJISTAR_OptInputs[] =
{ 
  &TA_DEF_UI_Penetration_30,
  NULL
};

DEF_FUNCTION( CDLEVENINGDOJISTAR,             /* name */
              TA_GroupId_PatternRecognition,  /* groupId */
              "Evening Doji Star",            /* hint */
              NULL,                           /* helpFile */
              TA_FUNC_FLG_CANDLESTICK,        /* flags */
              NULL                            /* analysis function */
             );

/* CDLEVENINGDOJISTAR END */

/* CDLEVENINGSTAR BEGIN */
static const TA_InputParameterInfo    *TA_CDLEVENINGSTAR_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_OHLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_CDLEVENINGSTAR_Outputs[]   =
{
  &TA_DEF_UI_Output_Integer,
  NULL
};

static const TA_OptInputParameterInfo *TA_CDLEVENINGSTAR_OptInputs[] =
{ 
  &TA_DEF_UI_Penetration_30,
  NULL
};

DEF_FUNCTION( CDLEVENINGSTAR,                 /* name */
              TA_GroupId_PatternRecognition,  /* groupId */
              "Evening Star",                 /* hint */
              NULL,                           /* helpFile */
              TA_FUNC_FLG_CANDLESTICK,        /* flags */
              NULL                            /* analysis function */
             );

/* CDLEVENINGSTAR END */

/* CDLGRAVESTONEDOJI BEGIN */
static const TA_InputParameterInfo    *TA_CDLGRAVESTONEDOJI_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_OHLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_CDLGRAVESTONEDOJI_Outputs[]   =
{
  &TA_DEF_UI_Output_Integer,
  NULL
};

static const TA_OptInputParameterInfo *TA_CDLGRAVESTONEDOJI_OptInputs[] =
{ 
  NULL
};

DEF_FUNCTION( CDLGRAVESTONEDOJI,              /* name */
              TA_GroupId_PatternRecognition,  /* groupId */
              "Gravestone Doji",              /* hint */
              NULL,                           /* helpFile */
              TA_FUNC_FLG_CANDLESTICK,        /* flags */
              NULL                            /* analysis function */
             );

/* CDLGRAVESTONEDOJI END */

/* CDLHAMMER BEGIN */
static const TA_InputParameterInfo    *TA_CDLHAMMER_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_OHLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_CDLHAMMER_Outputs[]   =
{
  &TA_DEF_UI_Output_Integer,
  NULL
};

static const TA_OptInputParameterInfo *TA_CDLHAMMER_OptInputs[] =
{ 
  NULL
};

DEF_FUNCTION( CDLHAMMER,                      /* name */
              TA_GroupId_PatternRecognition,  /* groupId */
              "Hammer",                       /* hint */
              NULL,                           /* helpFile */
              TA_FUNC_FLG_CANDLESTICK,        /* flags */
              NULL                            /* analysis function */
             );

/* CDLHAMMER END */

/* CDLHANGINGMAN BEGIN */
static const TA_InputParameterInfo    *TA_CDLHANGINGMAN_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_OHLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_CDLHANGINGMAN_Outputs[]   =
{
  &TA_DEF_UI_Output_Integer,
  NULL
};

static const TA_OptInputParameterInfo *TA_CDLHANGINGMAN_OptInputs[] =
{ 
  NULL
};

DEF_FUNCTION( CDLHANGINGMAN,                  /* name */
              TA_GroupId_PatternRecognition,  /* groupId */
              "Hanging Man",                  /* hint */
              NULL,                           /* helpFile */
              TA_FUNC_FLG_CANDLESTICK,        /* flags */
              NULL                            /* analysis function */
             );

/* CDLHANGINGMAN END */

/* CDLHARAMI BEGIN */
static const TA_InputParameterInfo    *TA_CDLHARAMI_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_OHLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_CDLHARAMI_Outputs[]   =
{
  &TA_DEF_UI_Output_Integer,
  NULL
};

static const TA_OptInputParameterInfo *TA_CDLHARAMI_OptInputs[] =
{ 
  NULL
};

DEF_FUNCTION( CDLHARAMI,                      /* name */
              TA_GroupId_PatternRecognition,  /* groupId */
              "Harami Pattern",               /* hint */
              NULL,                           /* helpFile */
              TA_FUNC_FLG_CANDLESTICK,        /* flags */
              NULL                            /* analysis function */
             );

/* CDLHARAMI END */

/* CDLHARAMICROSS BEGIN */
static const TA_InputParameterInfo    *TA_CDLHARAMICROSS_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_OHLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_CDLHARAMICROSS_Outputs[]   =
{
  &TA_DEF_UI_Output_Integer,
  NULL
};

static const TA_OptInputParameterInfo *TA_CDLHARAMICROSS_OptInputs[] =
{ 
  NULL
};

DEF_FUNCTION( CDLHARAMICROSS,                 /* name */
              TA_GroupId_PatternRecognition,  /* groupId */
              "Harami Cross Pattern",         /* hint */
              NULL,                           /* helpFile */
              TA_FUNC_FLG_CANDLESTICK,        /* flags */
              NULL                            /* analysis function */
             );

/* CDLHARAMICROSS END */

/* CDLHIGHWAVE BEGIN */
static const TA_InputParameterInfo    *TA_CDLHIGHWAVE_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_OHLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_CDLHIGHWAVE_Outputs[]   =
{
  &TA_DEF_UI_Output_Integer,
  NULL
};

static const TA_OptInputParameterInfo *TA_CDLHIGHWAVE_OptInputs[] =
{ 
  NULL
};

DEF_FUNCTION( CDLHIGHWAVE,                    /* name */
              TA_GroupId_PatternRecognition,  /* groupId */
              "High-Wave Candle",             /* hint */
              NULL,                           /* helpFile */
              TA_FUNC_FLG_CANDLESTICK,        /* flags */
              NULL                            /* analysis function */
             );

/* CDLHIGHWAVE END */

/* CDLIDENTICAL3CROWS BEGIN */
static const TA_InputParameterInfo    *TA_CDLIDENTICAL3CROWS_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_OHLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_CDLIDENTICAL3CROWS_Outputs[]   =
{
  &TA_DEF_UI_Output_Integer,
  NULL
};

static const TA_OptInputParameterInfo *TA_CDLIDENTICAL3CROWS_OptInputs[] =
{ 
  NULL
};

DEF_FUNCTION( CDLIDENTICAL3CROWS,             /* name */
              TA_GroupId_PatternRecognition,  /* groupId */
              "Identical Three Crows",        /* hint */
              NULL,                           /* helpFile */
              TA_FUNC_FLG_CANDLESTICK,        /* flags */
              NULL                            /* analysis function */
             );

/* CDLIDENTICAL3CROWS END */

/* CDLINNECK BEGIN */
static const TA_InputParameterInfo    *TA_CDLINNECK_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_OHLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_CDLINNECK_Outputs[]   =
{
  &TA_DEF_UI_Output_Integer,
  NULL
};

static const TA_OptInputParameterInfo *TA_CDLINNECK_OptInputs[] =
{ 
  NULL
};

DEF_FUNCTION( CDLINNECK,                      /* name */
              TA_GroupId_PatternRecognition,  /* groupId */
              "In-Neck Pattern",              /* hint */
              NULL,                           /* helpFile */
              TA_FUNC_FLG_CANDLESTICK,        /* flags */
              NULL                            /* analysis function */
             );

/* CDLINNECK END */

/* CDLINVERTEDHAMMER BEGIN */
static const TA_InputParameterInfo    *TA_CDLINVERTEDHAMMER_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_OHLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_CDLINVERTEDHAMMER_Outputs[]   =
{
  &TA_DEF_UI_Output_Integer,
  NULL
};

static const TA_OptInputParameterInfo *TA_CDLINVERTEDHAMMER_OptInputs[] =
{ 
  NULL
};

DEF_FUNCTION( CDLINVERTEDHAMMER,              /* name */
              TA_GroupId_PatternRecognition,  /* groupId */
              "Inverted Hammer",              /* hint */
              NULL,                           /* helpFile */
              TA_FUNC_FLG_CANDLESTICK,        /* flags */
              NULL                            /* analysis function */
             );

/* CDLINVERTEDHAMMER END */

/* CDLKICKING BEGIN */
static const TA_InputParameterInfo    *TA_CDLKICKING_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_OHLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_CDLKICKING_Outputs[]   =
{
  &TA_DEF_UI_Output_Integer,
  NULL
};

static const TA_OptInputParameterInfo *TA_CDLKICKING_OptInputs[] =
{ 
  NULL
};

DEF_FUNCTION( CDLKICKING,                     /* name */
              TA_GroupId_PatternRecognition,  /* groupId */
              "Kicking",                      /* hint */
              NULL,                           /* helpFile */
              TA_FUNC_FLG_CANDLESTICK,        /* flags */
              NULL                            /* analysis function */
             );

/* CDLKICKING END */

/* CDLKICKINGbyLength BEGIN */
static const TA_InputParameterInfo    *TA_CDLKICKINGbyLength_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_OHLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_CDLKICKINGbyLength_Outputs[]   =
{
  &TA_DEF_UI_Output_Integer,
  NULL
};

static const TA_OptInputParameterInfo *TA_CDLKICKINGbyLength_OptInputs[] =
{ 
  NULL
};

DEF_FUNCTION( CDLKICKINGbyLength,             /* name */
              TA_GroupId_PatternRecognition,  /* groupId */
              "Kicking - bull/bear determined by the longer marubozu",              /* hint */
              NULL,                           /* helpFile */
              TA_FUNC_FLG_CANDLESTICK,        /* flags */
              NULL                            /* analysis function */
             );

/* CDLKICKINGbyLength END */

/* CDLLONGLEGGEDDOJI BEGIN */
static const TA_InputParameterInfo    *TA_CDLLONGLEGGEDDOJI_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_OHLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_CDLLONGLEGGEDDOJI_Outputs[]   =
{
  &TA_DEF_UI_Output_Integer,
  NULL
};

static const TA_OptInputParameterInfo *TA_CDLLONGLEGGEDDOJI_OptInputs[] =
{ 
  NULL
};

DEF_FUNCTION( CDLLONGLEGGEDDOJI,              /* name */
              TA_GroupId_PatternRecognition,  /* groupId */
              "Long Legged Doji",             /* hint */
              NULL,                           /* helpFile */
              TA_FUNC_FLG_CANDLESTICK,        /* flags */
              NULL                            /* analysis function */
             );

/* CDLLONGLEGGEDDOJI END */

/* CDLLONGLINE BEGIN */
static const TA_InputParameterInfo    *TA_CDLLONGLINE_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_OHLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_CDLLONGLINE_Outputs[]   =
{
  &TA_DEF_UI_Output_Integer,
  NULL
};

static const TA_OptInputParameterInfo *TA_CDLLONGLINE_OptInputs[] =
{ 
  NULL
};

DEF_FUNCTION( CDLLONGLINE,                    /* name */
              TA_GroupId_PatternRecognition,  /* groupId */
              "Long Line Candle",             /* hint */
              NULL,                           /* helpFile */
              TA_FUNC_FLG_CANDLESTICK,        /* flags */
              NULL                            /* analysis function */
             );

/* CDLLONGLINE END */

/* CDLMARUBOZU BEGIN */
static const TA_InputParameterInfo    *TA_CDLMARUBOZU_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_OHLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_CDLMARUBOZU_Outputs[]   =
{
  &TA_DEF_UI_Output_Integer,
  NULL
};

static const TA_OptInputParameterInfo *TA_CDLMARUBOZU_OptInputs[] =
{ 
  NULL
};

DEF_FUNCTION( CDLMARUBOZU,                    /* name */
              TA_GroupId_PatternRecognition,  /* groupId */
              "Marubozu",                     /* hint */
              NULL,                           /* helpFile */
              TA_FUNC_FLG_CANDLESTICK,        /* flags */
              NULL                            /* analysis function */
             );

/* CDLMARUBOZU END */

/* CDLMORNINGDOJISTAR BEGIN */
static const TA_InputParameterInfo    *TA_CDLMORNINGDOJISTAR_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_OHLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_CDLMORNINGDOJISTAR_Outputs[]   =
{
  &TA_DEF_UI_Output_Integer,
  NULL
};

static const TA_OptInputParameterInfo *TA_CDLMORNINGDOJISTAR_OptInputs[] =
{ 
  &TA_DEF_UI_Penetration_30,
  NULL
};

DEF_FUNCTION( CDLMORNINGDOJISTAR,             /* name */
              TA_GroupId_PatternRecognition,  /* groupId */
              "Morning Doji Star",            /* hint */
              NULL,                           /* helpFile */
              TA_FUNC_FLG_CANDLESTICK,        /* flags */
              NULL                            /* analysis function */
             );

/* CDLMORNINGDOJISTAR END */

/* CDLMORNINGSTAR BEGIN */
static const TA_InputParameterInfo    *TA_CDLMORNINGSTAR_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_OHLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_CDLMORNINGSTAR_Outputs[]   =
{
  &TA_DEF_UI_Output_Integer,
  NULL
};

static const TA_OptInputParameterInfo *TA_CDLMORNINGSTAR_OptInputs[] =
{ 
  &TA_DEF_UI_Penetration_30,
  NULL
};

DEF_FUNCTION( CDLMORNINGSTAR,                 /* name */
              TA_GroupId_PatternRecognition,  /* groupId */
              "Morning Star",                 /* hint */
              NULL,                           /* helpFile */
              TA_FUNC_FLG_CANDLESTICK,        /* flags */
              NULL                            /* analysis function */
             );

/* CDLMORNINGSTAR END */

/* CDLONNECK BEGIN */
static const TA_InputParameterInfo    *TA_CDLONNECK_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_OHLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_CDLONNECK_Outputs[]   =
{
  &TA_DEF_UI_Output_Integer,
  NULL
};

static const TA_OptInputParameterInfo *TA_CDLONNECK_OptInputs[] =
{ 
  NULL
};

DEF_FUNCTION( CDLONNECK,                      /* name */
              TA_GroupId_PatternRecognition,  /* groupId */
              "On-Neck Pattern",              /* hint */
              NULL,                           /* helpFile */
              TA_FUNC_FLG_CANDLESTICK,        /* flags */
              NULL                            /* analysis function */
             );

/* CDLONNECK END */

/* CDLPIERCING BEGIN */
static const TA_InputParameterInfo    *TA_CDLPIERCING_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_OHLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_CDLPIERCING_Outputs[]   =
{
  &TA_DEF_UI_Output_Integer,
  NULL
};

static const TA_OptInputParameterInfo *TA_CDLPIERCING_OptInputs[] =
{ 
  NULL
};

DEF_FUNCTION( CDLPIERCING,                    /* name */
              TA_GroupId_PatternRecognition,  /* groupId */
              "Piercing Pattern",             /* hint */
              NULL,                           /* helpFile */
              TA_FUNC_FLG_CANDLESTICK,        /* flags */
              NULL                            /* analysis function */
             );

/* CDLPIERCING END */

/* CDLRICKSHAWMAN BEGIN */
static const TA_InputParameterInfo    *TA_CDLRICKSHAWMAN_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_OHLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_CDLRICKSHAWMAN_Outputs[]   =
{
  &TA_DEF_UI_Output_Integer,
  NULL
};

static const TA_OptInputParameterInfo *TA_CDLRICKSHAWMAN_OptInputs[] =
{ 
  NULL
};

DEF_FUNCTION( CDLRICKSHAWMAN,                 /* name */
              TA_GroupId_PatternRecognition,  /* groupId */
              "Rickshaw Man",                 /* hint */
              NULL,                           /* helpFile */
              TA_FUNC_FLG_CANDLESTICK,        /* flags */
              NULL                            /* analysis function */
             );

/* CDLRICKSHAWMAN END */

/* CDLSEPARATINGLINES BEGIN */
static const TA_InputParameterInfo    *TA_CDLSEPARATINGLINES_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_OHLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_CDLSEPARATINGLINES_Outputs[]   =
{
  &TA_DEF_UI_Output_Integer,
  NULL
};

static const TA_OptInputParameterInfo *TA_CDLSEPARATINGLINES_OptInputs[] =
{ 
  NULL
};

DEF_FUNCTION( CDLSEPARATINGLINES,             /* name */
              TA_GroupId_PatternRecognition,  /* groupId */
              "Separating Lines",             /* hint */
              NULL,                           /* helpFile */
              TA_FUNC_FLG_CANDLESTICK,        /* flags */
              NULL                            /* analysis function */
             );

/* CDLSEPARATINGLINES END */

/* CDLSHOOTINGSTAR BEGIN */
static const TA_InputParameterInfo    *TA_CDLSHOOTINGSTAR_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_OHLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_CDLSHOOTINGSTAR_Outputs[]   =
{
  &TA_DEF_UI_Output_Integer,
  NULL
};

static const TA_OptInputParameterInfo *TA_CDLSHOOTINGSTAR_OptInputs[] =
{ 
  NULL
};

DEF_FUNCTION( CDLSHOOTINGSTAR,                /* name */
              TA_GroupId_PatternRecognition,  /* groupId */
              "Shooting Star",                /* hint */
              NULL,                           /* helpFile */
              TA_FUNC_FLG_CANDLESTICK,        /* flags */
              NULL                            /* analysis function */
             );

/* CDLSHOOTINGSTAR END */

/* CDLSHORTLINE BEGIN */
static const TA_InputParameterInfo    *TA_CDLSHORTLINE_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_OHLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_CDLSHORTLINE_Outputs[]   =
{
  &TA_DEF_UI_Output_Integer,
  NULL
};

static const TA_OptInputParameterInfo *TA_CDLSHORTLINE_OptInputs[] =
{ 
  NULL
};

DEF_FUNCTION( CDLSHORTLINE,                   /* name */
              TA_GroupId_PatternRecognition,  /* groupId */
              "Short Line Candle",            /* hint */
              NULL,                           /* helpFile */
              TA_FUNC_FLG_CANDLESTICK,        /* flags */
              NULL                            /* analysis function */
             );

/* CDLSHORTLINE END */

/* CDLSPINNINGTOP BEGIN */
static const TA_InputParameterInfo    *TA_CDLSPINNINGTOP_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_OHLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_CDLSPINNINGTOP_Outputs[]   =
{
  &TA_DEF_UI_Output_Integer,
  NULL
};

static const TA_OptInputParameterInfo *TA_CDLSPINNINGTOP_OptInputs[] =
{ 
  NULL
};

DEF_FUNCTION( CDLSPINNINGTOP,                 /* name */
              TA_GroupId_PatternRecognition,  /* groupId */
              "Spinning Top",                 /* hint */
              NULL,                           /* helpFile */
              TA_FUNC_FLG_CANDLESTICK,        /* flags */
              NULL                            /* analysis function */
             );

/* CDLSPINNINGTOP END */

/* CDLSTALLEDPATTERN BEGIN */
static const TA_InputParameterInfo    *TA_CDLSTALLEDPATTERN_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_OHLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_CDLSTALLEDPATTERN_Outputs[]   =
{
  &TA_DEF_UI_Output_Integer,
  NULL
};

static const TA_OptInputParameterInfo *TA_CDLSTALLEDPATTERN_OptInputs[] =
{ 
  NULL
};

DEF_FUNCTION( CDLSTALLEDPATTERN,              /* name */
              TA_GroupId_PatternRecognition,  /* groupId */
              "Stalled Pattern",              /* hint */
              NULL,                           /* helpFile */
              TA_FUNC_FLG_CANDLESTICK,        /* flags */
              NULL                            /* analysis function */
             );

/* CDLSTALLEDPATTERN END */

/* CDLTAKURI BEGIN */
static const TA_InputParameterInfo    *TA_CDLTAKURI_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_OHLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_CDLTAKURI_Outputs[]   =
{
  &TA_DEF_UI_Output_Integer,
  NULL
};

static const TA_OptInputParameterInfo *TA_CDLTAKURI_OptInputs[] =
{ 
  NULL
};

DEF_FUNCTION( CDLTAKURI,                      /* name */
              TA_GroupId_PatternRecognition,  /* groupId */
              "Takuri (Dragonfly Doji with very long lower shadow)",    /* hint */
              NULL,                           /* helpFile */
              TA_FUNC_FLG_CANDLESTICK,        /* flags */
              NULL                            /* analysis function */
             );

/* CDLTAKURI END */

/* CDLTASUKIGAP BEGIN */
static const TA_InputParameterInfo    *TA_CDLTASUKIGAP_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_OHLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_CDLTASUKIGAP_Outputs[]   =
{
  &TA_DEF_UI_Output_Integer,
  NULL
};

static const TA_OptInputParameterInfo *TA_CDLTASUKIGAP_OptInputs[] =
{ 
  NULL
};

DEF_FUNCTION( CDLTASUKIGAP,                   /* name */
              TA_GroupId_PatternRecognition,  /* groupId */
              "Tasuki Gap",                   /* hint */
              NULL,                           /* helpFile */
              TA_FUNC_FLG_CANDLESTICK,        /* flags */
              NULL                            /* analysis function */
             );

/* CDLTASUKIGAP END */

/* CDLTHRUSTING BEGIN */
static const TA_InputParameterInfo    *TA_CDLTHRUSTING_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_OHLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_CDLTHRUSTING_Outputs[]   =
{
  &TA_DEF_UI_Output_Integer,
  NULL
};

static const TA_OptInputParameterInfo *TA_CDLTHRUSTING_OptInputs[] =
{ 
  NULL
};

DEF_FUNCTION( CDLTHRUSTING,                   /* name */
              TA_GroupId_PatternRecognition,  /* groupId */
              "Thrusting Pattern",            /* hint */
              NULL,                           /* helpFile */
              TA_FUNC_FLG_CANDLESTICK,        /* flags */
              NULL                            /* analysis function */
             );

/* CDLTHRUSTING END */

/* CDLTRISTAR BEGIN */
static const TA_InputParameterInfo    *TA_CDLTRISTAR_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_OHLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_CDLTRISTAR_Outputs[]   =
{
  &TA_DEF_UI_Output_Integer,
  NULL
};

static const TA_OptInputParameterInfo *TA_CDLTRISTAR_OptInputs[] =
{ 
  NULL
};

DEF_FUNCTION( CDLTRISTAR,                     /* name */
              TA_GroupId_PatternRecognition,  /* groupId */
              "Tristar Pattern",              /* hint */
              NULL,                           /* helpFile */
              TA_FUNC_FLG_CANDLESTICK,        /* flags */
              NULL                            /* analysis function */
             );

/* CDLTRISTAR END */

/* CDLUPSIDEGAP2CROWS BEGIN */
static const TA_InputParameterInfo    *TA_CDLUPSIDEGAP2CROWS_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_OHLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_CDLUPSIDEGAP2CROWS_Outputs[]   =
{
  &TA_DEF_UI_Output_Integer,
  NULL
};

static const TA_OptInputParameterInfo *TA_CDLUPSIDEGAP2CROWS_OptInputs[] =
{ 
  NULL
};

DEF_FUNCTION( CDLUPSIDEGAP2CROWS,             /* name */
              TA_GroupId_PatternRecognition,  /* groupId */
              "Upside Gap Two Crows",         /* hint */
              NULL,                           /* helpFile */
              TA_FUNC_FLG_CANDLESTICK,        /* flags */
              NULL                            /* analysis function */
             );

/* CDLUPSIDEGAP2CROWS END */

/* CDLXSIDEGAP3METHODS BEGIN */
static const TA_InputParameterInfo    *TA_CDLXSIDEGAP3METHODS_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_OHLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_CDLXSIDEGAP3METHODS_Outputs[]   =
{
  &TA_DEF_UI_Output_Integer,
  NULL
};

static const TA_OptInputParameterInfo *TA_CDLXSIDEGAP3METHODS_OptInputs[] =
{ 
  NULL
};

DEF_FUNCTION( CDLXSIDEGAP3METHODS,            /* name */
              TA_GroupId_PatternRecognition,  /* groupId */
              "Upside/Downside Gap Three Methods",    /* hint */
              NULL,                           /* helpFile */
              TA_FUNC_FLG_CANDLESTICK,        /* flags */
              NULL                            /* analysis function */
             );

/* CDLXSIDEGAP3METHODS END */

/* CORREL BEGIN */
static const TA_InputParameterInfo    *TA_CORREL_Inputs[]    =
{
  &TA_DEF_UI_Input_Real0,
  &TA_DEF_UI_Input_Real1,
  NULL
};

static const TA_OutputParameterInfo   *TA_CORREL_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_CORREL_OptInputs[] =
{ &TA_DEF_UI_TimePeriod_30,
  NULL
};

DEF_FUNCTION( CORREL,                      /* name */
              TA_GroupId_Statistic,     /* groupId */
              "Pearson's Correlation Coefficient (r)", /* hint */
              NULL,                     /* helpFile */
              0,                        /* flags */
              NULL                      /* analysis function */
             );
/* CORREL END */

/****************************************************************************
 * Step 2 - Add your TA function to the table.
 *          Order is not important. Must be NULL terminated.
 ****************************************************************************/
const TA_FuncDef *TA_DEF_TableC[] =
{
   ADD_TO_TABLE(CCI),
   ADD_TO_TABLE(CDL2CROWS),
   ADD_TO_TABLE(CDL3BLACKCROWS),
   ADD_TO_TABLE(CDL3INSIDE),
   ADD_TO_TABLE(CDL3LINESTRIKE),
   ADD_TO_TABLE(CDL3OUTSIDE),
   ADD_TO_TABLE(CDL3WHITESOLDIERS),
   ADD_TO_TABLE(CDLABANDONEDBABY),
   ADD_TO_TABLE(CDLADVANCEBLOCK),
   ADD_TO_TABLE(CDLBELTHOLD),
   ADD_TO_TABLE(CDLBREAKAWAY),
   ADD_TO_TABLE(CDLCOUNTERATTACK),
   ADD_TO_TABLE(CDLDARKCLOUDCOVER),
   ADD_TO_TABLE(CDLDOJI),
   ADD_TO_TABLE(CDLDOJISTAR),
   ADD_TO_TABLE(CDLDRAGONFLYDOJI),
   ADD_TO_TABLE(CDLENGULFING),
   ADD_TO_TABLE(CDLEVENINGDOJISTAR),
   ADD_TO_TABLE(CDLEVENINGSTAR),
   ADD_TO_TABLE(CDLGRAVESTONEDOJI),
   ADD_TO_TABLE(CDLHAMMER),
   ADD_TO_TABLE(CDLHANGINGMAN),
   ADD_TO_TABLE(CDLHARAMI),
   ADD_TO_TABLE(CDLHARAMICROSS),
   ADD_TO_TABLE(CDLHIGHWAVE),
   ADD_TO_TABLE(CDLIDENTICAL3CROWS),
   ADD_TO_TABLE(CDLINNECK),
   ADD_TO_TABLE(CDLINVERTEDHAMMER),
   ADD_TO_TABLE(CDLKICKING),
   ADD_TO_TABLE(CDLKICKINGbyLength),
   ADD_TO_TABLE(CDLLONGLEGGEDDOJI),
   ADD_TO_TABLE(CDLLONGLINE),
   ADD_TO_TABLE(CDLMARUBOZU),
   ADD_TO_TABLE(CDLMORNINGDOJISTAR),
   ADD_TO_TABLE(CDLMORNINGSTAR),
   ADD_TO_TABLE(CDLONNECK),
   ADD_TO_TABLE(CDLPIERCING),
   ADD_TO_TABLE(CDLRICKSHAWMAN),
   ADD_TO_TABLE(CDLSEPARATINGLINES),
   ADD_TO_TABLE(CDLSHOOTINGSTAR),
   ADD_TO_TABLE(CDLSHORTLINE),
   ADD_TO_TABLE(CDLSPINNINGTOP),  
   ADD_TO_TABLE(CDLSTALLEDPATTERN),  
   ADD_TO_TABLE(CDLTAKURI),
   ADD_TO_TABLE(CDLTASUKIGAP),
   ADD_TO_TABLE(CDLTHRUSTING),
   ADD_TO_TABLE(CDLTRISTAR),
   ADD_TO_TABLE(CDLUPSIDEGAP2CROWS), 
   ADD_TO_TABLE(CDLXSIDEGAP3METHODS), 
   ADD_TO_TABLE(CORREL),
   NULL
};


/* Do not modify the following line. */
const unsigned int TA_DEF_TableCSize =
              ((sizeof(TA_DEF_TableC)/sizeof(TA_FuncDef *))-1);


/****************************************************************************
 * Step 3 - Make sure "gen_code" is executed for generating all other
 *          source files derived from this one.
 *          You can then re-compile the library as usual and you are done!
 ****************************************************************************/
