/*********************************************************************
 * This file contains only TA functions starting with the letter 'D' *
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

/* DEMA BEGIN */
static const TA_InputParameterInfo    *TA_DEMA_Inputs[]    =
{
  &TA_DEF_UI_Input_Real,
  NULL
};

static const TA_OutputParameterInfo   *TA_DEMA_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_DEMA_OptInputs[] =
{ &TA_DEF_UI_TimePeriod_30,
  &TA_DEF_UI_Compatibility_CL_MS,
  NULL
};

DEF_FUNCTION( DEMA,                       /* name */
              TA_GroupId_OverlapStudies,  /* groupId */
              "Double Exponential Moving Average", /* hint */
              NULL,                       /* helpFile */
              TA_FUNC_FLG_OVERLAP,        /* flags */
              NULL                        /* analysis function */
             );
/* DEMA END */

/* DX BEGIN */
static const TA_InputParameterInfo    *TA_DX_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_HLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_DX_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_DX_OptInputs[] =
{ &TA_DEF_UI_TimePeriod_14,
  NULL
};

DEF_FUNCTION( DX,                           /* name */
              TA_GroupId_TrendIndicators,   /* groupId */
              "Directional Movement Index", /* hint */
              NULL,                         /* helpFile */
              TA_FUNC_FLG_UNST_PER,         /* flags */
              NULL                          /* analysis function */
             );
/* DX END */

/****************************************************************************
 * Step 3 - Add your TA function to the table.
 *          Order is not important. Must be NULL terminated.
 ****************************************************************************/
const TA_FuncDef *TA_DEF_TableD[] =
{
   ADD_TO_TABLE(DEMA),
   ADD_TO_TABLE(DX),
   NULL
};


/* Do not modify the following line. */
const unsigned int TA_DEF_TableDSize =
              ((sizeof(TA_DEF_TableD)/sizeof(TA_FuncDef *))-1);


/****************************************************************************
 * Step 4 - Make sure "gen_code" is executed for generating all other
 *          source files derived from this one.
 *          You can then re-compile the library as usual and you are done!
 ****************************************************************************/
