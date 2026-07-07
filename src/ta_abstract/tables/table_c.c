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
 * This file contains only TA functions starting with the letter 'C' *
 *********************************************************************/
#include <stddef.h>
#include "ta_abstract.h"
#include "ta_def_ui.h"

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

DEF_FUNCTION( CCI,
              TA_GroupId_MomentumIndicators,
              "Commodity Channel Index",
              "Cci",
              0
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
{ NULL };

DEF_FUNCTION( CDL2CROWS,
              TA_GroupId_PatternRecognition,
              "Two Crows",
              "Cdl2Crows",
              TA_FUNC_FLG_CANDLESTICK
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
{ NULL };

DEF_FUNCTION( CDL3BLACKCROWS,
              TA_GroupId_PatternRecognition,
              "Three Black Crows",
              "Cdl3BlackCrows",
              TA_FUNC_FLG_CANDLESTICK
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
{ NULL };

DEF_FUNCTION( CDL3INSIDE,
              TA_GroupId_PatternRecognition,
              "Three Inside Up/Down",
              "Cdl3Inside",
              TA_FUNC_FLG_CANDLESTICK
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
{ NULL };

DEF_FUNCTION( CDL3LINESTRIKE,
              TA_GroupId_PatternRecognition,
              "Three-Line Strike",
              "Cdl3LineStrike",
              TA_FUNC_FLG_CANDLESTICK
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
{ NULL };

DEF_FUNCTION( CDL3OUTSIDE,
              TA_GroupId_PatternRecognition,
              "Three Outside Up/Down",
              "Cdl3Outside",
              TA_FUNC_FLG_CANDLESTICK
             );
/* CDL3OUTSIDE END */

/* CDL3STARSINSOUTH BEGIN */
static const TA_InputParameterInfo    *TA_CDL3STARSINSOUTH_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_OHLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_CDL3STARSINSOUTH_Outputs[]   =
{
  &TA_DEF_UI_Output_Integer,
  NULL
};

static const TA_OptInputParameterInfo *TA_CDL3STARSINSOUTH_OptInputs[] =
{ NULL };

DEF_FUNCTION( CDL3STARSINSOUTH,
              TA_GroupId_PatternRecognition,
              "Three Stars In The South",
              "Cdl3StarsInSouth",
              TA_FUNC_FLG_CANDLESTICK
             );
/* CDL3STARSINSOUTH END */

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
{ NULL };

DEF_FUNCTION( CDL3WHITESOLDIERS,
              TA_GroupId_PatternRecognition,
              "Three Advancing White Soldiers",
              "Cdl3WhiteSoldiers",
              TA_FUNC_FLG_CANDLESTICK
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
{ &TA_DEF_UI_Penetration_30,
  NULL
};

DEF_FUNCTION( CDLABANDONEDBABY,
              TA_GroupId_PatternRecognition,
              "Abandoned Baby",
              "CdlAbandonedBaby",
              TA_FUNC_FLG_CANDLESTICK
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
{ NULL };

DEF_FUNCTION( CDLADVANCEBLOCK,
              TA_GroupId_PatternRecognition,
              "Advance Block",
              "CdlAdvanceBlock",
              TA_FUNC_FLG_CANDLESTICK
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
{ NULL };

DEF_FUNCTION( CDLBELTHOLD,
              TA_GroupId_PatternRecognition,
              "Belt-hold",
              "CdlBeltHold",
              TA_FUNC_FLG_CANDLESTICK
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
{ NULL };

DEF_FUNCTION( CDLBREAKAWAY,
              TA_GroupId_PatternRecognition,
              "Breakaway",
              "CdlBreakaway",
              TA_FUNC_FLG_CANDLESTICK
             );
/* CDLBREAKAWAY END */

/* CDLCLOSINGMARUBOZU BEGIN */
static const TA_InputParameterInfo    *TA_CDLCLOSINGMARUBOZU_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_OHLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_CDLCLOSINGMARUBOZU_Outputs[]   =
{
  &TA_DEF_UI_Output_Integer,
  NULL
};

static const TA_OptInputParameterInfo *TA_CDLCLOSINGMARUBOZU_OptInputs[] =
{ NULL };

DEF_FUNCTION( CDLCLOSINGMARUBOZU,
              TA_GroupId_PatternRecognition,
              "Closing Marubozu",
              "CdlClosingMarubozu",
              TA_FUNC_FLG_CANDLESTICK
             );
/* CDLCLOSINGMARUBOZU END */

/* CDLCONCEALBABYSWALL BEGIN */
static const TA_InputParameterInfo    *TA_CDLCONCEALBABYSWALL_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_OHLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_CDLCONCEALBABYSWALL_Outputs[]   =
{
  &TA_DEF_UI_Output_Integer,
  NULL
};

static const TA_OptInputParameterInfo *TA_CDLCONCEALBABYSWALL_OptInputs[] =
{ NULL };

DEF_FUNCTION( CDLCONCEALBABYSWALL,
              TA_GroupId_PatternRecognition,
              "Concealing Baby Swallow",
              "CdlConcealBabysWall",
              TA_FUNC_FLG_CANDLESTICK
             );
/* CDLCONCEALBABYSWALL END */

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
{ NULL };

DEF_FUNCTION( CDLCOUNTERATTACK,
              TA_GroupId_PatternRecognition,
              "Counterattack",
              "CdlCounterAttack",
              TA_FUNC_FLG_CANDLESTICK
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
{ &TA_DEF_UI_Penetration_50,
  NULL
};

DEF_FUNCTION( CDLDARKCLOUDCOVER,
              TA_GroupId_PatternRecognition,
              "Dark Cloud Cover",
              "CdlDarkCloudCover",
              TA_FUNC_FLG_CANDLESTICK
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
{ NULL };

DEF_FUNCTION( CDLDOJI,
              TA_GroupId_PatternRecognition,
              "Doji",
              "CdlDoji",
              TA_FUNC_FLG_CANDLESTICK
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
{ NULL };

DEF_FUNCTION( CDLDOJISTAR,
              TA_GroupId_PatternRecognition,
              "Doji Star",
              "CdlDojiStar",
              TA_FUNC_FLG_CANDLESTICK
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
{ NULL };

DEF_FUNCTION( CDLDRAGONFLYDOJI,
              TA_GroupId_PatternRecognition,
              "Dragonfly Doji",
              "CdlDragonflyDoji",
              TA_FUNC_FLG_CANDLESTICK
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
{ NULL };

DEF_FUNCTION( CDLENGULFING,
              TA_GroupId_PatternRecognition,
              "Engulfing Pattern",
              "CdlEngulfing",
              TA_FUNC_FLG_CANDLESTICK
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
{ &TA_DEF_UI_Penetration_30,
  NULL
};

DEF_FUNCTION( CDLEVENINGDOJISTAR,
              TA_GroupId_PatternRecognition,
              "Evening Doji Star",
              "CdlEveningDojiStar",
              TA_FUNC_FLG_CANDLESTICK
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
{ &TA_DEF_UI_Penetration_30,
  NULL
};

DEF_FUNCTION( CDLEVENINGSTAR,
              TA_GroupId_PatternRecognition,
              "Evening Star",
              "CdlEveningStar",
              TA_FUNC_FLG_CANDLESTICK
             );
/* CDLEVENINGSTAR END */

/* CDLGAPSIDESIDEWHITE BEGIN */
static const TA_InputParameterInfo    *TA_CDLGAPSIDESIDEWHITE_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_OHLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_CDLGAPSIDESIDEWHITE_Outputs[]   =
{
  &TA_DEF_UI_Output_Integer,
  NULL
};

static const TA_OptInputParameterInfo *TA_CDLGAPSIDESIDEWHITE_OptInputs[] =
{ NULL };

DEF_FUNCTION( CDLGAPSIDESIDEWHITE,
              TA_GroupId_PatternRecognition,
              "Up/Down-gap side-by-side white lines",
              "CdlGapSideSideWhite",
              TA_FUNC_FLG_CANDLESTICK
             );
/* CDLGAPSIDESIDEWHITE END */

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
{ NULL };

DEF_FUNCTION( CDLGRAVESTONEDOJI,
              TA_GroupId_PatternRecognition,
              "Gravestone Doji",
              "CdlGravestoneDoji",
              TA_FUNC_FLG_CANDLESTICK
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
{ NULL };

DEF_FUNCTION( CDLHAMMER,
              TA_GroupId_PatternRecognition,
              "Hammer",
              "CdlHammer",
              TA_FUNC_FLG_CANDLESTICK
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
{ NULL };

DEF_FUNCTION( CDLHANGINGMAN,
              TA_GroupId_PatternRecognition,
              "Hanging Man",
              "CdlHangingMan",
              TA_FUNC_FLG_CANDLESTICK
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
{ NULL };

DEF_FUNCTION( CDLHARAMI,
              TA_GroupId_PatternRecognition,
              "Harami Pattern",
              "CdlHarami",
              TA_FUNC_FLG_CANDLESTICK
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
{ NULL };

DEF_FUNCTION( CDLHARAMICROSS,
              TA_GroupId_PatternRecognition,
              "Harami Cross Pattern",
              "CdlHaramiCross",
              TA_FUNC_FLG_CANDLESTICK
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
{ NULL };

DEF_FUNCTION( CDLHIGHWAVE,
              TA_GroupId_PatternRecognition,
              "High-Wave Candle",
              "CdlHignWave",
              TA_FUNC_FLG_CANDLESTICK
             );
/* CDLHIGHWAVE END */

/* CDLHIKKAKE BEGIN */
static const TA_InputParameterInfo    *TA_CDLHIKKAKE_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_OHLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_CDLHIKKAKE_Outputs[]   =
{
  &TA_DEF_UI_Output_Integer,
  NULL
};

static const TA_OptInputParameterInfo *TA_CDLHIKKAKE_OptInputs[] =
{ NULL };

DEF_FUNCTION( CDLHIKKAKE,
              TA_GroupId_PatternRecognition,
              "Hikkake Pattern",
              "CdlHikkake",
              TA_FUNC_FLG_CANDLESTICK
             );
/* CDLHIKKAKE END */

/* CDLHIKKAKEMOD BEGIN */
static const TA_InputParameterInfo    *TA_CDLHIKKAKEMOD_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_OHLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_CDLHIKKAKEMOD_Outputs[]   =
{
  &TA_DEF_UI_Output_Integer,
  NULL
};

static const TA_OptInputParameterInfo *TA_CDLHIKKAKEMOD_OptInputs[] =
{ NULL };

DEF_FUNCTION( CDLHIKKAKEMOD,
              TA_GroupId_PatternRecognition,
              "Modified Hikkake Pattern",
              "CdlHikkakeMod",
              TA_FUNC_FLG_CANDLESTICK
             );
/* CDLHIKKAKEMOD END */

/* CDLHOMINGPIGEON BEGIN */
static const TA_InputParameterInfo    *TA_CDLHOMINGPIGEON_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_OHLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_CDLHOMINGPIGEON_Outputs[]   =
{
  &TA_DEF_UI_Output_Integer,
  NULL
};

static const TA_OptInputParameterInfo *TA_CDLHOMINGPIGEON_OptInputs[] =
{ NULL };

DEF_FUNCTION( CDLHOMINGPIGEON,
              TA_GroupId_PatternRecognition,
              "Homing Pigeon",
              "CdlHomingPigeon",
              TA_FUNC_FLG_CANDLESTICK
             );
/* CDLHOMINGPIGEON END */

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
{ NULL };

DEF_FUNCTION( CDLIDENTICAL3CROWS,
              TA_GroupId_PatternRecognition,
              "Identical Three Crows",
              "CdlIdentical3Crows",
              TA_FUNC_FLG_CANDLESTICK
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
{ NULL };

DEF_FUNCTION( CDLINNECK,
              TA_GroupId_PatternRecognition,
              "In-Neck Pattern",
              "CdlInNeck",
              TA_FUNC_FLG_CANDLESTICK
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
{ NULL };

DEF_FUNCTION( CDLINVERTEDHAMMER,
              TA_GroupId_PatternRecognition,
              "Inverted Hammer",
              "CdlInvertedHammer",
              TA_FUNC_FLG_CANDLESTICK
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
{ NULL };

DEF_FUNCTION( CDLKICKING,
              TA_GroupId_PatternRecognition,
              "Kicking",
              "CdlKicking",
              TA_FUNC_FLG_CANDLESTICK
             );
/* CDLKICKING END */

/* CDLKICKINGBYLENGTH BEGIN */
static const TA_InputParameterInfo    *TA_CDLKICKINGBYLENGTH_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_OHLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_CDLKICKINGBYLENGTH_Outputs[]   =
{
  &TA_DEF_UI_Output_Integer,
  NULL
};

static const TA_OptInputParameterInfo *TA_CDLKICKINGBYLENGTH_OptInputs[] =
{ NULL };

DEF_FUNCTION( CDLKICKINGBYLENGTH,
              TA_GroupId_PatternRecognition,
              "Kicking - bull/bear determined by the longer marubozu",
              "CdlKickingByLength",
              TA_FUNC_FLG_CANDLESTICK
             );
/* CDLKICKINGBYLENGTH END */

/* CDLLADDERBOTTOM BEGIN */
static const TA_InputParameterInfo    *TA_CDLLADDERBOTTOM_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_OHLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_CDLLADDERBOTTOM_Outputs[]   =
{
  &TA_DEF_UI_Output_Integer,
  NULL
};

static const TA_OptInputParameterInfo *TA_CDLLADDERBOTTOM_OptInputs[] =
{ NULL };

DEF_FUNCTION( CDLLADDERBOTTOM,
              TA_GroupId_PatternRecognition,
              "Ladder Bottom",
              "CdlLadderBottom",
              TA_FUNC_FLG_CANDLESTICK
             );
/* CDLLADDERBOTTOM END */

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
{ NULL };

DEF_FUNCTION( CDLLONGLEGGEDDOJI,
              TA_GroupId_PatternRecognition,
              "Long Legged Doji",
              "CdlLongLeggedDoji",
              TA_FUNC_FLG_CANDLESTICK
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
{ NULL };

DEF_FUNCTION( CDLLONGLINE,
              TA_GroupId_PatternRecognition,
              "Long Line Candle",
              "CdlLongLine",
              TA_FUNC_FLG_CANDLESTICK
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
{ NULL };

DEF_FUNCTION( CDLMARUBOZU,
              TA_GroupId_PatternRecognition,
              "Marubozu",
              "CdlMarubozu",
              TA_FUNC_FLG_CANDLESTICK
             );
/* CDLMARUBOZU END */

/* CDLMATCHINGLOW BEGIN */
static const TA_InputParameterInfo    *TA_CDLMATCHINGLOW_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_OHLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_CDLMATCHINGLOW_Outputs[]   =
{
  &TA_DEF_UI_Output_Integer,
  NULL
};

static const TA_OptInputParameterInfo *TA_CDLMATCHINGLOW_OptInputs[] =
{ NULL };

DEF_FUNCTION( CDLMATCHINGLOW,
              TA_GroupId_PatternRecognition,
              "Matching Low",
              "CdlMatchingLow",
              TA_FUNC_FLG_CANDLESTICK
             );
/* CDLMATCHINGLOW END */

/* CDLMATHOLD BEGIN */
static const TA_InputParameterInfo    *TA_CDLMATHOLD_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_OHLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_CDLMATHOLD_Outputs[]   =
{
  &TA_DEF_UI_Output_Integer,
  NULL
};

static const TA_OptInputParameterInfo *TA_CDLMATHOLD_OptInputs[] =
{ &TA_DEF_UI_Penetration_50,
  NULL
};

DEF_FUNCTION( CDLMATHOLD,
              TA_GroupId_PatternRecognition,
              "Mat Hold",
              "CdlMatHold",
              TA_FUNC_FLG_CANDLESTICK
             );
/* CDLMATHOLD END */

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
{ &TA_DEF_UI_Penetration_30,
  NULL
};

DEF_FUNCTION( CDLMORNINGDOJISTAR,
              TA_GroupId_PatternRecognition,
              "Morning Doji Star",
              "CdlMorningDojiStar",
              TA_FUNC_FLG_CANDLESTICK
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
{ &TA_DEF_UI_Penetration_30,
  NULL
};

DEF_FUNCTION( CDLMORNINGSTAR,
              TA_GroupId_PatternRecognition,
              "Morning Star",
              "CdlMorningStar",
              TA_FUNC_FLG_CANDLESTICK
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
{ NULL };

DEF_FUNCTION( CDLONNECK,
              TA_GroupId_PatternRecognition,
              "On-Neck Pattern",
              "CdlOnNeck",
              TA_FUNC_FLG_CANDLESTICK
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
{ NULL };

DEF_FUNCTION( CDLPIERCING,
              TA_GroupId_PatternRecognition,
              "Piercing Pattern",
              "CdlPiercing",
              TA_FUNC_FLG_CANDLESTICK
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
{ NULL };

DEF_FUNCTION( CDLRICKSHAWMAN,
              TA_GroupId_PatternRecognition,
              "Rickshaw Man",
              "CdlRickshawMan",
              TA_FUNC_FLG_CANDLESTICK
             );
/* CDLRICKSHAWMAN END */

/* CDLRISEFALL3METHODS BEGIN */
static const TA_InputParameterInfo    *TA_CDLRISEFALL3METHODS_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_OHLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_CDLRISEFALL3METHODS_Outputs[]   =
{
  &TA_DEF_UI_Output_Integer,
  NULL
};

static const TA_OptInputParameterInfo *TA_CDLRISEFALL3METHODS_OptInputs[] =
{ NULL };

DEF_FUNCTION( CDLRISEFALL3METHODS,
              TA_GroupId_PatternRecognition,
              "Rising/Falling Three Methods",
              "CdlRiseFall3Methods",
              TA_FUNC_FLG_CANDLESTICK
             );
/* CDLRISEFALL3METHODS END */

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
{ NULL };

DEF_FUNCTION( CDLSEPARATINGLINES,
              TA_GroupId_PatternRecognition,
              "Separating Lines",
              "CdlSeperatingLines",
              TA_FUNC_FLG_CANDLESTICK
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
{ NULL };

DEF_FUNCTION( CDLSHOOTINGSTAR,
              TA_GroupId_PatternRecognition,
              "Shooting Star",
              "CdlShootingStar",
              TA_FUNC_FLG_CANDLESTICK
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
{ NULL };

DEF_FUNCTION( CDLSHORTLINE,
              TA_GroupId_PatternRecognition,
              "Short Line Candle",
              "CdlShortLine",
              TA_FUNC_FLG_CANDLESTICK
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
{ NULL };

DEF_FUNCTION( CDLSPINNINGTOP,
              TA_GroupId_PatternRecognition,
              "Spinning Top",
              "CdlSpinningTop",
              TA_FUNC_FLG_CANDLESTICK
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
{ NULL };

DEF_FUNCTION( CDLSTALLEDPATTERN,
              TA_GroupId_PatternRecognition,
              "Stalled Pattern",
              "CdlStalledPattern",
              TA_FUNC_FLG_CANDLESTICK
             );
/* CDLSTALLEDPATTERN END */

/* CDLSTICKSANDWICH BEGIN */
static const TA_InputParameterInfo    *TA_CDLSTICKSANDWICH_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_OHLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_CDLSTICKSANDWICH_Outputs[]   =
{
  &TA_DEF_UI_Output_Integer,
  NULL
};

static const TA_OptInputParameterInfo *TA_CDLSTICKSANDWICH_OptInputs[] =
{ NULL };

DEF_FUNCTION( CDLSTICKSANDWICH,
              TA_GroupId_PatternRecognition,
              "Stick Sandwich",
              "CdlStickSandwich",
              TA_FUNC_FLG_CANDLESTICK
             );
/* CDLSTICKSANDWICH END */

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
{ NULL };

DEF_FUNCTION( CDLTAKURI,
              TA_GroupId_PatternRecognition,
              "Takuri (Dragonfly Doji with very long lower shadow)",
              "CdlTakuri",
              TA_FUNC_FLG_CANDLESTICK
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
{ NULL };

DEF_FUNCTION( CDLTASUKIGAP,
              TA_GroupId_PatternRecognition,
              "Tasuki Gap",
              "CdlTasukiGap",
              TA_FUNC_FLG_CANDLESTICK
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
{ NULL };

DEF_FUNCTION( CDLTHRUSTING,
              TA_GroupId_PatternRecognition,
              "Thrusting Pattern",
              "CdlThrusting",
              TA_FUNC_FLG_CANDLESTICK
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
{ NULL };

DEF_FUNCTION( CDLTRISTAR,
              TA_GroupId_PatternRecognition,
              "Tristar Pattern",
              "CdlTristar",
              TA_FUNC_FLG_CANDLESTICK
             );
/* CDLTRISTAR END */

/* CDLUNIQUE3RIVER BEGIN */
static const TA_InputParameterInfo    *TA_CDLUNIQUE3RIVER_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_OHLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_CDLUNIQUE3RIVER_Outputs[]   =
{
  &TA_DEF_UI_Output_Integer,
  NULL
};

static const TA_OptInputParameterInfo *TA_CDLUNIQUE3RIVER_OptInputs[] =
{ NULL };

DEF_FUNCTION( CDLUNIQUE3RIVER,
              TA_GroupId_PatternRecognition,
              "Unique 3 River",
              "CdlUnique3River",
              TA_FUNC_FLG_CANDLESTICK
             );
/* CDLUNIQUE3RIVER END */

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
{ NULL };

DEF_FUNCTION( CDLUPSIDEGAP2CROWS,
              TA_GroupId_PatternRecognition,
              "Upside Gap Two Crows",
              "CdlUpsideGap2Crows",
              TA_FUNC_FLG_CANDLESTICK
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
{ NULL };

DEF_FUNCTION( CDLXSIDEGAP3METHODS,
              TA_GroupId_PatternRecognition,
              "Upside/Downside Gap Three Methods",
              "CdlXSideGap3Methods",
              TA_FUNC_FLG_CANDLESTICK
             );
/* CDLXSIDEGAP3METHODS END */

/* CEIL BEGIN */
static const TA_InputParameterInfo    *TA_CEIL_Inputs[]    =
{
  &TA_DEF_UI_Input_Real,
  NULL
};

static const TA_OutputParameterInfo   *TA_CEIL_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_CEIL_OptInputs[] =
{ NULL };

DEF_FUNCTION( CEIL,
              TA_GroupId_MathTransform,
              "Vector Ceil",
              "Ceil",
              0
             );
/* CEIL END */

/* CMO BEGIN */
static const TA_InputParameterInfo    *TA_CMO_Inputs[]    =
{
  &TA_DEF_UI_Input_Real,
  NULL
};

static const TA_OutputParameterInfo   *TA_CMO_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_CMO_OptInputs[] =
{ &TA_DEF_UI_TimePeriod_14_MINIMUM2,
  NULL
};

DEF_FUNCTION( CMO,
              TA_GroupId_MomentumIndicators,
              "Chande Momentum Oscillator",
              "Cmo",
              TA_FUNC_FLG_UNST_PER
             );
/* CMO END */

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

DEF_FUNCTION( CORREL,
              TA_GroupId_Statistic,
              "Pearson's Correlation Coefficient (r)",
              "Correl",
              0
             );
/* CORREL END */

/* COS BEGIN */
static const TA_InputParameterInfo    *TA_COS_Inputs[]    =
{
  &TA_DEF_UI_Input_Real,
  NULL
};

static const TA_OutputParameterInfo   *TA_COS_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_COS_OptInputs[] =
{ NULL };

DEF_FUNCTION( COS,
              TA_GroupId_MathTransform,
              "Vector Trigonometric Cos",
              "Cos",
              0
             );
/* COS END */

/* COSH BEGIN */
static const TA_InputParameterInfo    *TA_COSH_Inputs[]    =
{
  &TA_DEF_UI_Input_Real,
  NULL
};

static const TA_OutputParameterInfo   *TA_COSH_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_COSH_OptInputs[] =
{ NULL };

DEF_FUNCTION( COSH,
              TA_GroupId_MathTransform,
              "Vector Trigonometric Cosh",
              "Cosh",
              0
             );
/* COSH END */

/****************************************************************************
 * Step 2 - Add your TA function to the table.
 *          Keep in alphabetical order. Must be NULL terminated.
 ****************************************************************************/
const TA_FuncDef *TA_DEF_TableC[] =
{
   ADD_TO_TABLE(CCI),
   ADD_TO_TABLE(CDL2CROWS),
   ADD_TO_TABLE(CDL3BLACKCROWS),
   ADD_TO_TABLE(CDL3INSIDE),
   ADD_TO_TABLE(CDL3LINESTRIKE),
   ADD_TO_TABLE(CDL3OUTSIDE),
   ADD_TO_TABLE(CDL3STARSINSOUTH),
   ADD_TO_TABLE(CDL3WHITESOLDIERS),
   ADD_TO_TABLE(CDLABANDONEDBABY),
   ADD_TO_TABLE(CDLADVANCEBLOCK),
   ADD_TO_TABLE(CDLBELTHOLD),
   ADD_TO_TABLE(CDLBREAKAWAY),
   ADD_TO_TABLE(CDLCLOSINGMARUBOZU),
   ADD_TO_TABLE(CDLCONCEALBABYSWALL),
   ADD_TO_TABLE(CDLCOUNTERATTACK),
   ADD_TO_TABLE(CDLDARKCLOUDCOVER),
   ADD_TO_TABLE(CDLDOJI),
   ADD_TO_TABLE(CDLDOJISTAR),
   ADD_TO_TABLE(CDLDRAGONFLYDOJI),
   ADD_TO_TABLE(CDLENGULFING),
   ADD_TO_TABLE(CDLEVENINGDOJISTAR),
   ADD_TO_TABLE(CDLEVENINGSTAR),
   ADD_TO_TABLE(CDLGAPSIDESIDEWHITE),
   ADD_TO_TABLE(CDLGRAVESTONEDOJI),
   ADD_TO_TABLE(CDLHAMMER),
   ADD_TO_TABLE(CDLHANGINGMAN),
   ADD_TO_TABLE(CDLHARAMI),
   ADD_TO_TABLE(CDLHARAMICROSS),
   ADD_TO_TABLE(CDLHIGHWAVE),
   ADD_TO_TABLE(CDLHIKKAKE),
   ADD_TO_TABLE(CDLHIKKAKEMOD),
   ADD_TO_TABLE(CDLHOMINGPIGEON),
   ADD_TO_TABLE(CDLIDENTICAL3CROWS),
   ADD_TO_TABLE(CDLINNECK),
   ADD_TO_TABLE(CDLINVERTEDHAMMER),
   ADD_TO_TABLE(CDLKICKING),
   ADD_TO_TABLE(CDLKICKINGBYLENGTH),
   ADD_TO_TABLE(CDLLADDERBOTTOM),
   ADD_TO_TABLE(CDLLONGLEGGEDDOJI),
   ADD_TO_TABLE(CDLLONGLINE),
   ADD_TO_TABLE(CDLMARUBOZU),
   ADD_TO_TABLE(CDLMATCHINGLOW),
   ADD_TO_TABLE(CDLMATHOLD),
   ADD_TO_TABLE(CDLMORNINGDOJISTAR),
   ADD_TO_TABLE(CDLMORNINGSTAR),
   ADD_TO_TABLE(CDLONNECK),
   ADD_TO_TABLE(CDLPIERCING),
   ADD_TO_TABLE(CDLRICKSHAWMAN),
   ADD_TO_TABLE(CDLRISEFALL3METHODS),
   ADD_TO_TABLE(CDLSEPARATINGLINES),
   ADD_TO_TABLE(CDLSHOOTINGSTAR),
   ADD_TO_TABLE(CDLSHORTLINE),
   ADD_TO_TABLE(CDLSPINNINGTOP),
   ADD_TO_TABLE(CDLSTALLEDPATTERN),
   ADD_TO_TABLE(CDLSTICKSANDWICH),
   ADD_TO_TABLE(CDLTAKURI),
   ADD_TO_TABLE(CDLTASUKIGAP),
   ADD_TO_TABLE(CDLTHRUSTING),
   ADD_TO_TABLE(CDLTRISTAR),
   ADD_TO_TABLE(CDLUNIQUE3RIVER),
   ADD_TO_TABLE(CDLUPSIDEGAP2CROWS),
   ADD_TO_TABLE(CDLXSIDEGAP3METHODS),
   ADD_TO_TABLE(CEIL),
   ADD_TO_TABLE(CMO),
   ADD_TO_TABLE(CORREL),
   ADD_TO_TABLE(COS),
   ADD_TO_TABLE(COSH),
   NULL
};


/* Do not modify the following line. */
const unsigned int TA_DEF_TableCSize =
              ((sizeof(TA_DEF_TableC)/sizeof(TA_FuncDef *))-1);

