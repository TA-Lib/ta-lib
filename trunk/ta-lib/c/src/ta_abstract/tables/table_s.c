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

static const TA_OptInputParameterInfo TA_DEF_UI_FastK_Period =
{
   TA_OptInput_IntegerRange, /* type */
   "optInFastK_Period",           /* paramName */
   0,                        /* flags */

   "Fast-K Period", /* displayName */
   (const void *)&TA_DEF_TimePeriod_Positive, /* dataSet */
   5, /* defaultValue */
   "Time period for building the Fast-K line", /* hint */

   NULL /* helpFile */
};

static const TA_OptInputParameterInfo TA_DEF_UI_SlowK_Period =
{
   TA_OptInput_IntegerRange, /* type */
   "optInSlowK_Period",       /* paramName */
   0,                        /* flags */

   "Slow-K Period",     /* displayName */
   (const void *)&TA_DEF_TimePeriod_Positive, /* dataSet */
   3, /* defaultValue */
   "Smoothing for making the Slow-K line. Usually set to 3", /* hint */

   NULL /* helpFile */
};

static const TA_OptInputParameterInfo TA_DEF_UI_FastD_Period =
{
   TA_OptInput_IntegerRange, /* type */
   "optInFastD_Period",       /* paramName */
   0,                        /* flags */

   "Fast-D Period",     /* displayName */
   (const void *)&TA_DEF_TimePeriod_Positive, /* dataSet */
   3, /* defaultValue */
   "Smoothing for making the Fast-D line. Usually set to 3", /* hint */

   NULL /* helpFile */
};

static const TA_OptInputParameterInfo TA_DEF_UI_SlowD_Period =
{
   TA_OptInput_IntegerRange, /* type */
   "optInSlowD_Period",           /* paramName */
   0,                        /* flags */

   "Slow-D Period",            /* displayName */
   (const void *)&TA_DEF_TimePeriod_Positive, /* dataSet */
   3, /* defaultValue */
   "Smoothing for making the Slow-D line", /* hint */

   NULL /* helpFile */
};

static const TA_RealRange TA_DEF_AccelerationFactor =
{
   TA_REAL_MIN,  /* min */
   TA_REAL_MAX,  /* max */
   4,      /* precision */
   0.01,  /* suggested start */
   0.20,  /* suggested end   */
   0.01   /* suggested increment */
};

static const TA_RealRange TA_DEF_AccelerationMax =
{
   TA_REAL_MIN, /* min */
   TA_REAL_MAX, /* max */
   4,     /* precision */
   0.1,  /* suggested start */
   1.0,  /* suggested end   */
   0.1   /* suggested increment */
};

static const TA_OptInputParameterInfo TA_DEF_UI_D_AccelerationFactor =
{
   TA_OptInput_RealRange, /* type */
   "optInAcceleration",  /* paramName */
   0,          /* flags */

   "Acceleration Factor", /* displayName */
   (const void *)&TA_DEF_AccelerationFactor, /* dataSet */
   0.02, /* defaultValue */
   "Acceleration Factor used up to the Maximum parameter", /* hint */
   NULL /* helpFile */
};

static const TA_OptInputParameterInfo TA_DEF_UI_D_AccelerationMaximum =
{
   TA_OptInput_RealRange, /* type */
   "optInMaximum",        /* paramName */
   0,                     /* flags */

   "Acceleration Maximum", /* displayName */
   (const void *)&TA_DEF_AccelerationMax, /* dataSet */
   0.20, /* defaultValue */
   "Maximum value for the acceleration factor", /* hint */

   NULL /* helpFile */
};

const TA_OptInputParameterInfo TA_DEF_UI_SlowK_MAType =
{
   TA_OptInput_IntegerList, /* type */
   "optInSlowK_MAType",     /* paramName */
   0,                       /* flags */

   "Slow-K MA",                /* displayName */
   (const void *)&TA_MA_TypeList, /* dataSet */
   0, /* defaultValue = simple average */
   "Type of Moving Average for Slow-K", /* hint */

   NULL /* helpFile */
};

const TA_OptInputParameterInfo TA_DEF_UI_SlowD_MAType =
{
   TA_OptInput_IntegerList, /* type */
   "optInSlowD_MAType",     /* paramName */
   0,                       /* flags */

   "Slow-D MA",                /* displayName */
   (const void *)&TA_MA_TypeList, /* dataSet */
   0, /* defaultValue = simple average */
   "Type of Moving Average for Slow-D", /* hint */

   NULL /* helpFile */
};

const TA_OptInputParameterInfo TA_DEF_UI_FastD_MAType =
{
   TA_OptInput_IntegerList, /* type */
   "optInFastD_MAType",     /* paramName */
   0,                       /* flags */

   "Fast-D MA",                /* displayName */
   (const void *)&TA_MA_TypeList, /* dataSet */
   0, /* defaultValue = simple average */
   "Type of Moving Average for Fast-D", /* hint */

   NULL /* helpFile */
};


const TA_OutputParameterInfo TA_DEF_UI_Output_SlowK =
                               { TA_Output_Real, "outSlowK", TA_OUT_DASH_LINE };

const TA_OutputParameterInfo TA_DEF_UI_Output_SlowD =
                               { TA_Output_Real, "outSlowD", TA_OUT_DASH_LINE };

const TA_OutputParameterInfo TA_DEF_UI_Output_FastK =
                               { TA_Output_Real, "outFastK", TA_OUT_LINE };

const TA_OutputParameterInfo TA_DEF_UI_Output_FastD =
                               { TA_Output_Real, "outFastD", TA_OUT_LINE };

/****************************************************************************
 * Step 2 - Define here the interface to your TA functions with
 *          the macro DEF_FUNCTION.
 *
 ****************************************************************************/

/* SAR BEGIN */
static const TA_InputParameterInfo    *TA_SAR_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_HL,
  NULL
};

static const TA_OutputParameterInfo   *TA_SAR_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_SAR_OptInputs[] =
{ &TA_DEF_UI_D_AccelerationFactor,
  &TA_DEF_UI_D_AccelerationMaximum,
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
  &TA_DEF_UI_Output_SlowK,
  &TA_DEF_UI_Output_SlowD,
  NULL
};

static const TA_OptInputParameterInfo *TA_STOCH_OptInputs[] =
{ &TA_DEF_UI_FastK_Period,
  &TA_DEF_UI_SlowK_Period,
  &TA_DEF_UI_SlowK_MAType,
  &TA_DEF_UI_SlowD_Period,
  &TA_DEF_UI_SlowD_MAType,
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

/* STOCHF BEGIN */
static const TA_InputParameterInfo    *TA_STOCHF_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_HLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_STOCHF_Outputs[]   =
{
  &TA_DEF_UI_Output_FastK,
  &TA_DEF_UI_Output_FastD,
  NULL
};

static const TA_OptInputParameterInfo *TA_STOCHF_OptInputs[] =
{ &TA_DEF_UI_FastK_Period,
  &TA_DEF_UI_FastD_Period,
  &TA_DEF_UI_FastD_MAType,
  NULL
};

DEF_FUNCTION( STOCHF,                   /* name */
              TA_GroupId_MomentumIndicators, /* groupId */
              "Stochastic Fast",        /* hint */
              NULL,                     /* helpFile */
              0,                        /* flags */
              NULL                      /* analysis function */
             );
/* STOCHF END */

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
   ADD_TO_TABLE(STOCHF),
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
