/*********************************************************************
 * This file contains only TA functions starting with the letter 'T' *
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

/* None */

/****************************************************************************
 * Step 2 - Define here the interface to your TA functions with
 *          the macro DEF_FUNCTION.
 *
 ****************************************************************************/

/* T3 BEGIN */
static const TA_OptInputParameterInfo TA_DEF_UI_VFactor =
{
   TA_OptInput_RealRange, /* type */
   "optInVFactor", /* paramName */
   0,                  /* flags */

   "Volume Factor",          /* displayName */
   (const void *)&TA_DEF_ZeroToOne, /* dataSet */
   0.7, /* defaultValue */
   "Volume Factor", /* hint */
   NULL /* helpFile */
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
{ &TA_DEF_UI_TimePeriod_5_MINIMUM2,
  &TA_DEF_UI_VFactor,
  NULL
};

DEF_FUNCTION( T3,                       /* name */
              TA_GroupId_OverlapStudies,  /* groupId */
              "Triple Exponential Moving Average (T3)", /* hint */
              NULL,                       /* helpFile */
              TA_FUNC_FLG_OVERLAP|TA_FUNC_FLG_UNST_PER, /* flags */
              NULL                        /* analysis function */
             );
/* T3 END */

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
{ &TA_DEF_UI_TimePeriod_30_MINIMUM2,
  NULL
};

DEF_FUNCTION( TEMA,                       /* name */
              TA_GroupId_OverlapStudies,  /* groupId */
              "Triple Exponential Moving Average", /* hint */
              NULL,                       /* helpFile */
              TA_FUNC_FLG_OVERLAP,        /* flags */
              NULL                        /* analysis function */
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

static const TA_OptInputParameterInfo *TA_TRANGE_OptInputs[] = { NULL };

DEF_FUNCTION( TRANGE,                     /* name */
              TA_GroupId_VolatilityIndicators,  /* groupId */
              "True Range",               /* hint */
              NULL,                       /* helpFile */
              0,                          /* flags */
              NULL                        /* analysis function */
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
{ &TA_DEF_UI_TimePeriod_30_MINIMUM2,
  NULL
};

DEF_FUNCTION( TRIMA,                       /* name */
              TA_GroupId_OverlapStudies,  /* groupId */
              "Triangular Moving Average", /* hint */
              NULL,                       /* helpFile */
              TA_FUNC_FLG_OVERLAP,        /* flags */
              NULL                        /* analysis function */
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

DEF_FUNCTION( TRIX,                       /* name */
              TA_GroupId_MomentumIndicators,  /* groupId */
              "1-day Rate-Of-Change (ROC) of a Triple Smooth EMA", /* hint */
              NULL,                       /* helpFile */
              0,        /* flags */
              NULL                        /* analysis function */
             );
/* TRIX END */

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

static const TA_OptInputParameterInfo *TA_TYPPRICE_OptInputs[] = { NULL };

DEF_FUNCTION( TYPPRICE,                   /* name */
              TA_GroupId_PriceTransform,  /* groupId */
              "Typical Price",            /* hint */
              NULL,                       /* helpFile */
              TA_FUNC_FLG_OVERLAP,        /* flags */
              NULL                        /* analysis function */
             );
/* TYPPRICE END */

/****************************************************************************
 * Step 3 - Add your TA function to the table.
 *          Order is not important. Must be NULL terminated.
 ****************************************************************************/
const TA_FuncDef *TA_DEF_TableT[] =
{
   ADD_TO_TABLE(T3),
   ADD_TO_TABLE(TEMA),
   ADD_TO_TABLE(TRANGE),
   ADD_TO_TABLE(TRIMA),
   ADD_TO_TABLE(TRIX),
   ADD_TO_TABLE(TYPPRICE),
   NULL
};


/* Do not modify the following line. */
const unsigned int TA_DEF_TableTSize =
              ((sizeof(TA_DEF_TableT)/sizeof(TA_FuncDef *))-1);


/****************************************************************************
 * Step 4 - Make sure "gen_code" is executed for generating all other
 *          source files derived from this one.
 *          You can then re-compile the library as usual and you are done!
 ****************************************************************************/
