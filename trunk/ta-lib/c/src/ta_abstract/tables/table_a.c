/*********************************************************************
 * This file contains only TA functions starting with the letter 'A' *
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

const TA_OutputParameterInfo TA_DEF_UI_Output_Real_AroonUp =
                               { TA_Output_Real, "outAroonDown", TA_OUT_DASH_LINE };

const TA_OutputParameterInfo TA_DEF_UI_Output_Real_AroonDown =
                                { TA_Output_Real, "outAroonUp", TA_OUT_LINE };

/****************************************************************************
 * Step 2 - Define here the interface to your TA functions with
 *          the macro DEF_FUNCTION.
 *
 ****************************************************************************/

/* AD BEGIN */
static const TA_InputParameterInfo    *TA_AD_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_HLCV,
  NULL
};

static const TA_OutputParameterInfo   *TA_AD_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_AD_OptInputs[] =
{
  NULL
};

DEF_FUNCTION( AD,                         /* name */
              TA_GroupId_VolumeIndicators,   /* groupId */
              "Chaikin A/D Line", /* hint */
              NULL,                         /* helpFile */
              0,                            /* flags */
              NULL                          /* analysis function */
             );
/* AD END */

/* ADOSC BEGIN */
static const TA_InputParameterInfo    *TA_ADOSC_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_HLCV,
  NULL
};

static const TA_OutputParameterInfo   *TA_ADOSC_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_ADOSC_OptInputs[] =
{
  &TA_DEF_UI_TimePeriod_21_MINIMUM2,
  NULL
};

DEF_FUNCTION( ADOSC,                         /* name */
              TA_GroupId_VolumeIndicators,   /* groupId */
              "Chaikin A/D Oscillator", /* hint */
              NULL,                         /* helpFile */
              0,                            /* flags */
              NULL                          /* analysis function */
             );
/* ADOSC END */

/* ADX BEGIN */
static const TA_InputParameterInfo    *TA_ADX_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_HLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_ADX_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_ADX_OptInputs[] =
{ &TA_DEF_UI_TimePeriod_14_MINIMUM2,
  NULL
};

DEF_FUNCTION( ADX,                          /* name */
              TA_GroupId_MomentumIndicators,   /* groupId */
              "Average Directional Movement Index", /* hint */
              NULL,                         /* helpFile */
              TA_FUNC_FLG_UNST_PER,         /* flags */
              NULL                          /* analysis function */
             );
/* ADX END */

/* ADXR BEGIN */
static const TA_InputParameterInfo    *TA_ADXR_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_HLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_ADXR_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_ADXR_OptInputs[] =
{ &TA_DEF_UI_TimePeriod_14_MINIMUM2,
  NULL
};

DEF_FUNCTION( ADXR,                         /* name */
              TA_GroupId_MomentumIndicators,   /* groupId */
              "Average Directional Movement Index Rating", /* hint */
              NULL,                         /* helpFile */
              TA_FUNC_FLG_UNST_PER,         /* flags */
              NULL                          /* analysis function */
             );
/* ADXR END */

/* APO BEGIN */
static const TA_InputParameterInfo *TA_APO_Inputs[] =
{
  &TA_DEF_UI_Input_Real,
  NULL
};

static const TA_OutputParameterInfo   *TA_APO_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_APO_OptInputs[] =
{ &TA_DEF_UI_Fast_Period,
  &TA_DEF_UI_Slow_Period,
  &TA_DEF_UI_MA_Method,
  NULL
};

DEF_FUNCTION( APO,                         /* name */
              TA_GroupId_MomentumIndicators,  /* groupId */
              "Absolute Price Oscillator", /* hint */
              NULL,                        /* helpFile */
              0,                           /* flags */
              NULL                         /* analysis function */
             );
/* APO END */

/* AROON BEGIN */
static const TA_InputParameterInfo    *TA_AROON_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_HL,
  NULL
};

static const TA_OutputParameterInfo   *TA_AROON_Outputs[]   =
{
  &TA_DEF_UI_Output_Real_AroonUp,
  &TA_DEF_UI_Output_Real_AroonDown,
  NULL
};

static const TA_OptInputParameterInfo *TA_AROON_OptInputs[] = 
{ 
  &TA_DEF_UI_TimePeriod_14_MINIMUM2,
  NULL
};

DEF_FUNCTION( AROON,                          /* name */
              TA_GroupId_MomentumIndicators,  /* groupId */
              "Aroon",                        /* hint */
              NULL,                           /* helpFile */
              0,                              /* flags */
              NULL                            /* analysis function */
             );

/* AROON END */

/* AROONOSC BEGIN */
static const TA_InputParameterInfo    *TA_AROONOSC_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_HL,
  NULL
};

static const TA_OutputParameterInfo   *TA_AROONOSC_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_AROONOSC_OptInputs[] = 
{ 
  &TA_DEF_UI_TimePeriod_14_MINIMUM2,
  NULL
};

DEF_FUNCTION( AROONOSC,                       /* name */
              TA_GroupId_MomentumIndicators,  /* groupId */
              "Aroon Oscillator",             /* hint */
              NULL,                           /* helpFile */
              0,                              /* flags */
              NULL                            /* analysis function */
             );

/* AROONOSC END */

/* ATR BEGIN */
static const TA_InputParameterInfo    *TA_ATR_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_HLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_ATR_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_ATR_OptInputs[] =
{ &TA_DEF_UI_TimePeriod_14,
  NULL
};

DEF_FUNCTION( ATR,                        /* name */
              TA_GroupId_VolatilityIndicators, /* groupId */
              "Average True Range",       /* hint */
              NULL,                       /* helpFile */
              TA_FUNC_FLG_UNST_PER,       /* flags */
              NULL                        /* analysis function */
             );
/* ATR END */

/* AVGPRICE BEGIN */
static const TA_InputParameterInfo    *TA_AVGPRICE_Inputs[]    =
{
  &TA_DEF_UI_Input_Price_OHLC,
  NULL
};

static const TA_OutputParameterInfo   *TA_AVGPRICE_Outputs[]   =
{
  &TA_DEF_UI_Output_Real,
  NULL
};

static const TA_OptInputParameterInfo *TA_AVGPRICE_OptInputs[] = { NULL };

DEF_FUNCTION( AVGPRICE,                   /* name */
              TA_GroupId_PriceTransform,  /* groupId */
              "Average Price",            /* hint */
              NULL,                       /* helpFile */
              TA_FUNC_FLG_OVERLAP,        /* flags */
              NULL                        /* analysis function */
             );
/* AVGPRICE END */

/****************************************************************************
 * Step 3 - Add your TA function to the table.
 *          Order is not important. Must be NULL terminated.
 ****************************************************************************/
const TA_FuncDef *TA_DEF_TableA[] =
{
   ADD_TO_TABLE(AD),
   ADD_TO_TABLE(ADOSC),
   ADD_TO_TABLE(ADX),
   ADD_TO_TABLE(ADXR),
   ADD_TO_TABLE(APO),
   ADD_TO_TABLE(AROON),
   ADD_TO_TABLE(AROONOSC),
   ADD_TO_TABLE(ATR),
   ADD_TO_TABLE(AVGPRICE),
   NULL
};


/* Do not modify the following line. */
const unsigned int TA_DEF_TableASize =
              ((sizeof(TA_DEF_TableA)/sizeof(TA_FuncDef *))-1);


/****************************************************************************
 * Step 4 - Make sure "gen_code" is executed for generating all other
 *          source files derived from this one.
 *          You can then re-compile the library as usual and you are done!
 ****************************************************************************/
