/*********************************************************************
 * This file contains only TA functions starting with the letter 'S' *
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

const TA_OptInputParameterInfo TA_DEF_UI_K_Period =
{
   TA_OptInput_IntegerRange, /* type */
   "optInKPeriod",           /* paramName */
   0,                        /* flags */

   "K Periods",            /* displayName */
   (const void *)&TA_DEF_TimePeriod_Positive, /* dataSet */
   5, /* defaultValue */
   "Time periods for the stochastic %K line", /* hint */

   NULL /* helpFile */
};

const TA_OptInputParameterInfo TA_DEF_UI_K_SlowPeriod =
{
   TA_OptInput_IntegerRange, /* type */
   "optInKSlowPeriod",       /* paramName */
   0,                        /* flags */

   "K-Slow Periods",     /* displayName */
   (const void *)&TA_DEF_TimePeriod_Positive, /* dataSet */
   3, /* defaultValue */
   "Internal smoothing for the %K line. Usually between 1 to 3", /* hint */

   NULL /* helpFile */
};

const TA_OptInputParameterInfo TA_DEF_UI_D_SlowPeriod =
{
   TA_OptInput_IntegerRange, /* type */
   "optInDSlowPeriod",           /* paramName */
   0,                        /* flags */

   "D-Slow Periods",            /* displayName */
   (const void *)&TA_DEF_TimePeriod_Positive, /* dataSet */
   3, /* defaultValue */
   "Time periods for the moving average of the %K line. That average is the %D line", /* hint */

   NULL /* helpFile */
};


const TA_OutputParameterInfo TA_DEF_UI_Output_Real_K_Line =
                               { TA_Output_Real, "outRealK", TA_OUT_LINE };

const TA_OutputParameterInfo TA_DEF_UI_Output_Real_D_Line  =
                               { TA_Output_Real, "outRealD", TA_OUT_DASH_LINE };

/****************************************************************************
 * Step 2 - Define here the interface to your TA functions with
 *          the macro DEF_FUNCTION.
 *
 ****************************************************************************/

/* SAR BEGIN */
static const TA_InputParameterInfo    *TA_SAR_Inputs[]    =
{
  &TA_DEF_UI_Input_Real,
  NULL
};

static const TA_OutputParameterInfo   *TA_SAR_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_SAR_OptInputs[] =
{ &TA_DEF_UI_TimePeriod_30,
  NULL
};

DEF_FUNCTION( SAR,                        /* name */
              TA_GroupId_OverlapStudies,  /* groupId */
              "Parabolic SAR",            /* hint */
              NULL,                       /* helpFile */
              TA_FUNC_FLG_OVERLAP,        /* flags */
              NULL                        /* analysis function */
             );

/* SAR END */

/* SMA BEGIN */
static const TA_InputParameterInfo    *TA_SMA_Inputs[]    =
{
  &TA_DEF_UI_Input_Real,
  NULL
};

static const TA_OutputParameterInfo   *TA_SMA_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_SMA_OptInputs[] =
{ &TA_DEF_UI_TimePeriod_30,
  NULL
};

DEF_FUNCTION( SMA,                        /* name */
              TA_GroupId_OverlapStudies,  /* groupId */
              "Simple Moving Average",    /* hint */
              NULL,                       /* helpFile */
              TA_FUNC_FLG_OVERLAP,        /* flags */
              NULL                        /* analysis function */
             );

/* SMA END */

/* STDDEV BEGIN */
static const TA_InputParameterInfo    *TA_STDDEV_Inputs[]    =
{
  &TA_DEF_UI_Input_Real,
  NULL
};

static const TA_OutputParameterInfo   *TA_STDDEV_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_STDDEV_OptInputs[] =
{ &TA_DEF_UI_TimePeriod_5,
  &TA_DEF_UI_NbDeviation,
  NULL
};

DEF_FUNCTION( STDDEV,                   /* name */
              TA_GroupId_Statistic,     /* groupId */
              "Standard Deviation",     /* hint */
              NULL,                     /* helpFile */
              0,                        /* flags */
              NULL                      /* analysis function */
             );
/* STDDEV END */

/* STOCH BEGIN */
static const TA_InputParameterInfo    *TA_STOCH_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_HLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_STOCH_Outputs[]   =
{
  &TA_DEF_UI_Output_Real_K_Line,
  &TA_DEF_UI_Output_Real_D_Line,
  NULL
};

static const TA_OptInputParameterInfo *TA_STOCH_OptInputs[] =
{ &TA_DEF_UI_K_Period,
  &TA_DEF_UI_K_SlowPeriod,
  &TA_DEF_UI_D_SlowPeriod,
  &TA_DEF_UI_MA_Method,
  NULL
};

DEF_FUNCTION( STOCH,                   /* name */
              TA_GroupId_MomentumIndicators, /* groupId */
              "Stochastic",             /* hint */
              NULL,                     /* helpFile */
              0,                        /* flags */
              NULL                      /* analysis function */
             );
/* STOCH END */

/****************************************************************************
 * Step 3 - Add your TA function to the table.
 *          Order is not important. Must be NULL terminated.
 ****************************************************************************/
const TA_FuncDef *TA_DEF_TableS[] =
{
   ADD_TO_TABLE(SAR),
   ADD_TO_TABLE(STDDEV),
   ADD_TO_TABLE(SMA),
   ADD_TO_TABLE(STOCH),
   NULL
};


/* Do not modify the following line. */
const unsigned int TA_DEF_TableSSize =
              ((sizeof(TA_DEF_TableS)/sizeof(TA_FuncDef *))-1);


/****************************************************************************
 * Step 4 - Make sure "gen_code" is executed for generating all other
 *          source files derived from this one.
 *          You can then re-compile the library as usual and you are done!
 ****************************************************************************/
