/*********************************************************************
 * This file contains only TA functions starting with the letter 'W' *
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

/* WCL BEGIN */
static const TA_InputParameterInfo    *TA_WCLPRICE_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_HLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_WCLPRICE_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_WCLPRICE_OptInputs[] = { NULL };

DEF_FUNCTION( WCLPRICE,                   /* name */
              TA_GroupId_PriceTransform,  /* groupId */
              "Weighted Close Price",     /* hint */
              NULL,                       /* helpFile */
              TA_FUNC_FLG_OVERLAP,        /* flags */
              NULL                        /* analysis function */
             );
/* WCL END */

/* WILLR BEGIN */
static const TA_InputParameterInfo    *TA_WILLR_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_HLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_WILLR_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_WILLR_OptInputs[] =
{ &TA_DEF_UI_TimePeriod_14_MINIMUM2,
  NULL
};

DEF_FUNCTION( WILLR,          /* name */
              TA_GroupId_MomentumIndicators,  /* groupId */
              "Williams' %R", /* hint */
              NULL,           /* helpFile */
              0,              /* flags */
              NULL            /* analysis function */
             );
/* WILLR END */

/* WMA BEGIN */
static const TA_InputParameterInfo    *TA_WMA_Inputs[]    =
{
  &TA_DEF_UI_Input_Real,
  NULL
};

static const TA_OutputParameterInfo   *TA_WMA_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_WMA_OptInputs[] =
{ &TA_DEF_UI_TimePeriod_30_MINIMUM2,
  NULL
};

DEF_FUNCTION( WMA,                        /* name */
              TA_GroupId_OverlapStudies,  /* groupId */
              "Weighted Moving Average",  /* hint */
              NULL,                       /* helpFile */
              TA_FUNC_FLG_OVERLAP,        /* flags */
              NULL                        /* analysis function */
             );

/* WMA END */

/****************************************************************************
 * Step 3 - Add your TA function to the table.
 *          Order is not important. Must be NULL terminated.
 ****************************************************************************/
const TA_FuncDef *TA_DEF_TableW[] =
{
   ADD_TO_TABLE(WCLPRICE),
   ADD_TO_TABLE(WILLR),
   ADD_TO_TABLE(WMA),
   NULL
};


/* Do not modify the following line. */
const unsigned int TA_DEF_TableWSize =
              ((sizeof(TA_DEF_TableW)/sizeof(TA_FuncDef *))-1);


/****************************************************************************
 * Step 4 - Make sure "gen_code" is executed for generating all other
 *          source files derived from this one.
 *          You can then re-compile the library as usual and you are done!
 ****************************************************************************/
