/*********************************************************************
 * This file contains only TA functions starting with the letter 'M' *
 *********************************************************************/
#include <stddef.h>
#include "ta_abstract.h"
#include "ta_def_ui.h"

/* Follow the 4 steps defined below for adding a new TA Function to this
 * file.
 */

/***************************************************************************
 * Step 1 - Define user inputs that are particular to your function.
 *          Consider the ones already defined in "ta_def_ui.c".
 ***************************************************************************/

const TA_OptInputParameterInfo TA_DEF_UI_Signal_Period =
{
   TA_OptInput_IntegerRange, /* type */
   "optInSignalPeriod",      /* paramName */
   0,                        /* flags */

   "Signal Period",            /* displayName */
   (const void *)&TA_DEF_TimePeriod_Positive, /* dataSet */
   9, /* defaultValue */
   "Smoothing for the signal line (nb of period)", /* hint */

   NULL /* helpFile */
};

const TA_OutputParameterInfo TA_DEF_UI_Output_Real_MACD =
                               { TA_Output_Real, "outRealMACD", TA_OUT_LINE };

const TA_OutputParameterInfo TA_DEF_UI_Output_Real_MACDSignal =
                               { TA_Output_Real, "outRealMACDSignal", TA_OUT_DASH_LINE };

const TA_OutputParameterInfo TA_DEF_UI_Output_Real_MACDHist =
                                { TA_Output_Real, "outRealMACDHist", TA_OUT_HISTO };

/****************************************************************************
 * Step 2 - Define here the interface to your TA functions with
 *          the macro DEF_FUNCTION.
 *
 ****************************************************************************/

/* MA BEGIN */
static const TA_InputParameterInfo    *TA_MA_Inputs[]    =
{
  &TA_DEF_UI_Input_Real,
  NULL
};

static const TA_OutputParameterInfo   *TA_MA_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_MA_OptInputs[] =
{ &TA_DEF_UI_TimePeriod_30,
  &TA_DEF_UI_MA_Method,
  &TA_DEF_UI_Compatibility_CL_MS,
  NULL
};

DEF_FUNCTION( MA,                         /* name */
              TA_GroupId_OverlapStudies,  /* groupId */
              "All Moving Average",       /* hint */
              NULL,                       /* helpFile */
              TA_FUNC_FLG_OVERLAP,        /* flags */
              NULL                        /* analysis function */
             );
/* MA END */

/* MACD BEGIN */
static const TA_InputParameterInfo    *TA_MACD_Inputs[]    =
{
  &TA_DEF_UI_Input_Real,
  NULL
};

static const TA_OutputParameterInfo *TA_MACD_Outputs[]   =
{
  &TA_DEF_UI_Output_Real_MACD,
  &TA_DEF_UI_Output_Real_MACDSignal,
  &TA_DEF_UI_Output_Real_MACDHist,
  NULL
};

static const TA_OptInputParameterInfo *TA_MACD_OptInputs[] =
{ &TA_DEF_UI_Fast_Period,
  &TA_DEF_UI_Slow_Period,
  &TA_DEF_UI_Signal_Period,
  &TA_DEF_UI_Compatibility_CL_MS,
  NULL
};

DEF_FUNCTION( MACD,                       /* name */
              TA_GroupId_TrendIndicators,  /* groupId */
              "Moving Average Convergence/Divergence", /* hint */
              NULL,                       /* helpFile */
              0,                          /* flags */
              NULL                        /* analysis function */
             );
/* MACD END */

/* MACDFIX BEGIN */
static const TA_InputParameterInfo    *TA_MACDFIX_Inputs[]    =
{
  &TA_DEF_UI_Input_Real,
  NULL
};

static const TA_OutputParameterInfo   *TA_MACDFIX_Outputs[]   =
{
  &TA_DEF_UI_Output_Real_MACD,
  &TA_DEF_UI_Output_Real_MACDSignal,
  &TA_DEF_UI_Output_Real_MACDHist,
  NULL
};

static const TA_OptInputParameterInfo *TA_MACDFIX_OptInputs[] =
{ &TA_DEF_UI_Signal_Period,
  &TA_DEF_UI_Compatibility_CL_MS,
  NULL
};

DEF_FUNCTION( MACDFIX,                    /* name */
              TA_GroupId_TrendIndicators,  /* groupId */
              "Moving Average Convergence/Divergence Fix 12/26", /* hint */
              NULL,                       /* helpFile */
              0,                          /* flags */
              NULL                        /* analysis function */
             );
/* MACDFIX END */

/* MAX BEGIN */
static const TA_InputParameterInfo    *TA_MAX_Inputs[]    =
{
  &TA_DEF_UI_Input_Real,
  NULL
};

static const TA_OutputParameterInfo   *TA_MAX_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_MAX_OptInputs[] =
{ &TA_DEF_UI_TimePeriod_30,
  NULL
};

DEF_FUNCTION( MAX,                       /* name */
              TA_GroupId_MathOperators,  /* groupId */
              "Highest value over a specified period", /* hint */
              NULL,                       /* helpFile */
              TA_FUNC_FLG_OVERLAP,        /* flags */
              NULL                        /* analysis function */
             );
/* MAX END */

/* MEDPRICE BEGIN */
static const TA_InputParameterInfo    *TA_MEDPRICE_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_HL,
  NULL
};

static const TA_OutputParameterInfo   *TA_MEDPRICE_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_MEDPRICE_OptInputs[] = { NULL };

DEF_FUNCTION( MEDPRICE,                   /* name */
              TA_GroupId_PriceTransform,  /* groupId */
              "Median Price",             /* hint */
              NULL,                       /* helpFile */
              TA_FUNC_FLG_OVERLAP,        /* flags */
              NULL                        /* analysis function */
             );

/* MEDPRICE END */

/* MIN BEGIN */
static const TA_InputParameterInfo    *TA_MIN_Inputs[]    =
{
  &TA_DEF_UI_Input_Real,
  NULL
};

static const TA_OutputParameterInfo   *TA_MIN_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_MIN_OptInputs[] =
{ &TA_DEF_UI_TimePeriod_30,
  NULL
};

DEF_FUNCTION( MIN,                       /* name */
              TA_GroupId_MathOperators,  /* groupId */
              "Lowest value over a specified period", /* hint */
              NULL,                       /* helpFile */
              TA_FUNC_FLG_OVERLAP,        /* flags */
              NULL                        /* analysis function */
             );
/* MIN END */

/* MINUS_DI BEGIN */
static const TA_InputParameterInfo    *TA_MINUS_DI_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_HLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_MINUS_DI_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_MINUS_DI_OptInputs[] =
{ &TA_DEF_UI_TimePeriod_14,
  NULL
};

DEF_FUNCTION( MINUS_DI,                      /* name */
              TA_GroupId_TrendIndicators,    /* groupId */
              "Minus Directional Indicator", /* hint */
              NULL,                          /* helpFile */
              TA_FUNC_FLG_UNST_PER,          /* flags */
              NULL                           /* analysis function */
             );

/* MINUS_DI END */

/* MINUS_DM BEGIN */
static const TA_InputParameterInfo    *TA_MINUS_DM_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_HL,
  NULL
};

static const TA_OutputParameterInfo   *TA_MINUS_DM_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_MINUS_DM_OptInputs[] =
{ &TA_DEF_UI_TimePeriod_14,
  NULL
};

DEF_FUNCTION( MINUS_DM,                     /* name */
              TA_GroupId_TrendIndicators,   /* groupId */
              "Minus Directional Movement", /* hint */
              NULL,                         /* helpFile */
              TA_FUNC_FLG_UNST_PER,         /* flags */
              NULL                          /* analysis function */
             );

/* MINUS_DM END */

/* MOM BEGIN */
static const TA_InputParameterInfo    *TA_MOM_Inputs[]    =
{
  &TA_DEF_UI_Input_Real,
  NULL
};

static const TA_OutputParameterInfo   *TA_MOM_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_MOM_OptInputs[] =
{ &TA_DEF_UI_TimePeriod_10,
  NULL
};

DEF_FUNCTION( MOM,                     /* name */
              TA_GroupId_MomentumIndicators,  /* groupId */
              "Momentum",       /* hint */
              NULL,             /* helpFile */
              0,                /* flags */
              NULL              /* analysis function */
             );
/* MOM END */

/****************************************************************************
 * Step 3 - Add your TA function to the table.
 *          Order is not important. Must be NULL terminated.
 ****************************************************************************/
const TA_FuncDef *TA_DEF_TableM[] =
{
   ADD_TO_TABLE(MA),
   ADD_TO_TABLE(MACD),
   ADD_TO_TABLE(MACDFIX),
   ADD_TO_TABLE(MAX),
   ADD_TO_TABLE(MEDPRICE),
   ADD_TO_TABLE(MIN),
   ADD_TO_TABLE(MINUS_DI),
   ADD_TO_TABLE(MINUS_DM),
   ADD_TO_TABLE(MOM),
   NULL
};


/* Do not modify the following line. */
const unsigned int TA_DEF_TableMSize =
              ((sizeof(TA_DEF_TableM)/sizeof(TA_FuncDef *))-1);


/****************************************************************************
 * Step 4 - Make sure "gen_code" is executed for generating all other
 *          source files derived from this one.
 *          You can then re-compile the library as usual and you are done!
 ****************************************************************************/
