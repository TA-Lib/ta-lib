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

/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  112703 MF   First version.
 *  030104 MF   Add tests for TA_GetLookback
 *  062504 MF   Add test_default_calls.
 */

/* Description:
 *         Regression testing of the functionality provided
 *         by the ta_abstract module.
 */

/**** Headers ****/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "ta_test_priv.h"
#include "ta_trace.h"
#include "trionan.h"

/**** External functions declarations. ****/
/* None */

/**** External variables declarations. ****/
/* None */

/**** Global variables definitions.    ****/
/* None */

/**** Local declarations.              ****/
/* None */

/**** Local functions declarations.    ****/
static ErrorNumber testLookback(TA_ParamHolder *paramHolder );
static ErrorNumber test_default_calls(void);
static ErrorNumber callWithDefaults( const char *funcName,
									 const double *input,
									 const int *input_int, int size );

/**** Local variables definitions.     ****/
static double inputNegData[100];
static double inputZeroData[100];
static double inputRandFltEpsilon[100];
static double inputRandDblEpsilon[100];
static double inputRandomData[2000];

static int    inputNegData_int[100];
static int    inputZeroData_int[100];
static int    inputRandFltEpsilon_int[100];
static int    inputRandDblEpsilon_int[100];
static int    inputRandomData_int[2000];

static double output[10][2000];
static int    output_int[10][2000];

/**** Global functions definitions.   ****/
ErrorNumber test_abstract( void )
{
   ErrorNumber retValue;
   TA_RetCode retCode;
   TA_ParamHolder *paramHolder;
   const TA_FuncHandle *handle;
   TA_UDBase *udb;

   printf( "Testing Abstract interface\n" );
   
   retValue = allocLib( &udb );
   if( retValue != TA_TEST_PASS )
      return retValue;    

   /* Verify TA_GetLookback. */
   retCode = TA_GetFuncHandle( "STOCH", &handle );
   if( retCode != TA_SUCCESS )
   {
      printf( "Can't get the function handle [%d]\n", retCode );
      return TA_ABS_TST_FAIL_GETFUNCHANDLE;   
   }
                             
   retCode = TA_ParamHolderAlloc( handle, &paramHolder );
   if( retCode != TA_SUCCESS )
   {
      printf( "Can't allocate the param holder [%d]\n", retCode );
      return TA_ABS_TST_FAIL_PARAMHOLDERALLOC;
   }

   retValue = testLookback(paramHolder);
   if( retValue != TA_SUCCESS )
   {
      printf( "testLookback() failed [%d]\n", retValue );
      TA_ParamHolderFree( paramHolder );
      return retValue;
   }      

   retCode = TA_ParamHolderFree( paramHolder );
   if( retCode != TA_SUCCESS )
   {
      printf( "TA_ParamHolderFree failed [%d]\n", retCode );
      return TA_ABS_TST_FAIL_PARAMHOLDERFREE;
   }

   retValue = freeLib( udb );
   if( retValue != TA_TEST_PASS )
      return retValue;

   /* Call all the TA functions through the abstract interface. */
   retValue = allocLib( &udb );
   if( retValue != TA_TEST_PASS )
      return retValue;

   retValue = test_default_calls();
   if( retValue != TA_TEST_PASS )
   {
      printf( "TA-Abstract default call failed\n" );
      return retValue;
   }

   retValue = freeLib( udb );
   if( retValue != TA_TEST_PASS )
      return retValue;
   
   return TA_TEST_PASS; /* Succcess. */
}

/**** Local functions definitions.     ****/
static ErrorNumber testLookback( TA_ParamHolder *paramHolder )
{
  TA_RetCode retCode;
  int lookback;

  /* Change the parameters of STOCH and verify that TA_GetLookback respond correctly. */
  retCode = TA_SetOptInputParamInteger( paramHolder, 0, 3 );
  if( retCode != TA_SUCCESS )
  {
     printf( "TA_SetOptInputParamInteger call failed [%d]\n", retCode );
     return TA_ABS_TST_FAIL_OPTINPUTPARAMINTEGER;
  }

  retCode = TA_SetOptInputParamInteger( paramHolder, 1, 4 );
  if( retCode != TA_SUCCESS )
  {
     printf( "TA_SetOptInputParamInteger call failed [%d]\n", retCode );
     return TA_ABS_TST_FAIL_OPTINPUTPARAMINTEGER;
  }

  retCode = TA_SetOptInputParamInteger( paramHolder, 2, (TA_Integer)TA_MAType_SMA );
  if( retCode != TA_SUCCESS )
  {
     printf( "TA_SetOptInputParamInteger call failed [%d]\n", retCode );
     return TA_ABS_TST_FAIL_OPTINPUTPARAMINTEGER;
  }

  retCode = TA_SetOptInputParamInteger( paramHolder, 3, 4 );
  if( retCode != TA_SUCCESS )
  {
     printf( "TA_SetOptInputParamInteger call failed [%d]\n", retCode );
     return TA_ABS_TST_FAIL_OPTINPUTPARAMINTEGER;
  }

  retCode = TA_SetOptInputParamInteger( paramHolder, 4, (TA_Integer)TA_MAType_SMA );
  if( retCode != TA_SUCCESS )
  {
     printf( "TA_SetOptInputParamInteger call failed [%d]\n", retCode );
     return TA_ABS_TST_FAIL_OPTINPUTPARAMINTEGER;
  }

  retCode = TA_GetLookback(paramHolder,&lookback);
  if( retCode != TA_SUCCESS )
  {
     printf( "TA_GetLookback failed [%d]\n", retCode );
     return TA_ABS_TST_FAIL_GETLOOKBACK_CALL_1;
  }

  if( lookback != 8 )
  {
     printf( "TA_GetLookback failed [%d != 8]\n", lookback );
     return TA_ABS_TST_FAIL_GETLOOKBACK_1;
  }

  /* Change one parameter and check again. */
  retCode = TA_SetOptInputParamInteger( paramHolder, 3, 3 );
  if( retCode != TA_SUCCESS )
  {
     printf( "TA_SetOptInputParamInteger call failed [%d]\n", retCode );
     return TA_ABS_TST_FAIL_OPTINPUTPARAMINTEGER;
  }

  retCode = TA_GetLookback(paramHolder,&lookback);
  if( retCode != TA_SUCCESS )
  {
     printf( "TA_GetLookback failed [%d]\n", retCode );
     return TA_ABS_TST_FAIL_GETLOOKBACK_CALL_2;
  }

  if( lookback != 7 )
  {
     printf( "TA_GetLookback failed [%d != 7]\n", lookback );
     return TA_ABS_TST_FAIL_GETLOOKBACK_2;
  }
  
  return TA_TEST_PASS;
}


static void testDefault( const TA_FuncInfo *funcInfo, void *opaqueData )
{
   ErrorNumber *errorNumber;
   errorNumber = (ErrorNumber *)opaqueData;
   if( *errorNumber != TA_TEST_PASS )
      return;

#define CALL(x) { \
	*errorNumber = callWithDefaults( funcInfo->name, x, x##_int, sizeof(x)/sizeof(double) ); \
	if( *errorNumber != TA_TEST_PASS ) { \
	   printf( "Failed for [%s][%s]\n", funcInfo->name, #x ); \
       return; \
	} \
}
   CALL( inputNegData );
   CALL( inputZeroData );
   CALL( inputRandomData );
   CALL( inputRandFltEpsilon );
   CALL( inputRandDblEpsilon );

#undef CALL
}

static ErrorNumber callWithDefaults( const char *funcName, const double *input, const int *input_int, int size )
{
   TA_ParamHolder *paramHolder;
   const TA_FuncHandle *handle;
   const TA_FuncInfo *funcInfo;
   const TA_InputParameterInfo *inputInfo;
   const TA_OutputParameterInfo *outputInfo;

   TA_RetCode retCode;
   unsigned int i;
   int j;
   int outBegIdx, outNbElement;

   retCode = TA_GetFuncHandle( funcName, &handle );
   if( retCode != TA_SUCCESS )
   {
      printf( "Can't get the function handle [%d]\n", retCode );
      return TA_ABS_TST_FAIL_GETFUNCHANDLE;   
   }
                             
   retCode = TA_ParamHolderAlloc( handle, &paramHolder );
   if( retCode != TA_SUCCESS )
   {
      printf( "Can't allocate the param holder [%d]\n", retCode );
      return TA_ABS_TST_FAIL_PARAMHOLDERALLOC;
   }

   TA_GetFuncInfo( handle, &funcInfo );

   for( i=0; i < funcInfo->nbInput; i++ )
   {
      TA_GetInputParameterInfo( handle, i, &inputInfo );
	  switch(inputInfo->type)
	  {
	  case TA_Input_Price:
         TA_SetInputParamPricePtr( paramHolder, i, NULL,
			 inputInfo->flags&TA_IN_PRICE_OPEN?input:NULL,
			 inputInfo->flags&TA_IN_PRICE_HIGH?input:NULL,
			 inputInfo->flags&TA_IN_PRICE_LOW?input:NULL,
			 inputInfo->flags&TA_IN_PRICE_CLOSE?input:NULL,
			 inputInfo->flags&TA_IN_PRICE_VOLUME?input_int:NULL, NULL );
		 break;
	  case TA_Input_Real:
         TA_SetInputParamRealPtr( paramHolder, i, input );
		 break;
	  case TA_Input_Integer:
         TA_SetInputParamIntegerPtr( paramHolder, i, input_int );
         break;
	  }
   }

   for( i=0; i < funcInfo->nbOutput; i++ )
   {
      TA_GetOutputParameterInfo( handle, i, &outputInfo );
	  switch(outputInfo->type)
	  {
	  case TA_Output_Real:
	     TA_SetOutputParamRealPtr(paramHolder,i,&output[i][0]);         
         for( j=0; j < 2000; j++ )
            output[i][j] = TA_REAL_MIN;
		 break;
	  case TA_Output_Integer:
	     TA_SetOutputParamIntegerPtr(paramHolder,i,&output_int[i][0]);
         for( j=0; j < 2000; j++ )
            output_int[i][j] = TA_INTEGER_MIN;
		 break;
	  }
   }

   retCode = TA_CallFunc(paramHolder,0,size-1,&outBegIdx,&outNbElement);
   if( retCode != TA_SUCCESS )
   {
      printf( "TA_CallFunc() failed zero data test [%d]\n", retCode );
      TA_ParamHolderFree( paramHolder );
      return TA_ABS_TST_FAIL_CALLFUNC_1;
   }      

   for( i=0; i < funcInfo->nbOutput; i++ )
   {
	  switch(outputInfo->type)
	  {
	  case TA_Output_Real:	     
		for( j=0; j < outNbElement; j++ )
		{
			if( trio_isnan(output[i][j]) ||
                trio_isinf(output[i][j]) )
			{
				printf( "Failed for output[%d][%d] = %e\n", i, j, output[i][j] );
				return TA_ABS_TST_FAIL_INVALID_OUTPUT;
			}
		}
		break;
	  case TA_Output_Integer:	     
		break;
	  }
   }

   retCode = TA_ParamHolderFree( paramHolder );
   if( retCode != TA_SUCCESS )
   {
      printf( "TA_ParamHolderFree failed [%d]\n", retCode );
      return TA_ABS_TST_FAIL_PARAMHOLDERFREE;
   }

   return TA_TEST_PASS;
}

static ErrorNumber test_default_calls(void)
{
   ErrorNumber errNumber;
   unsigned int i;
   unsigned int sign;

   errNumber = TA_TEST_PASS;

   for( i=0; i < sizeof(inputNegData)/sizeof(double); i++ )
   {
      inputNegData[i] = -((double)((int)i));
	  inputNegData_int[i] = -(int)i;
   }

   for( i=0; i < sizeof(inputZeroData)/sizeof(double); i++ )
   {
      inputZeroData[i] = 0.0;
	  inputZeroData_int[i] = (int)inputZeroData[i];
   }

   for( i=0; i < sizeof(inputRandomData)/sizeof(double); i++ )
   {
      inputRandomData[i] = (double)rand()/97.234;
	  inputRandomData_int[i] = (int)inputRandomData[i];
   }

   for( i=0; i < sizeof(inputRandFltEpsilon)/sizeof(double); i++ )
   {
       sign= (unsigned int)rand()%2;
       inputRandFltEpsilon[i] = (sign?1.0:-1.0)*(FLT_EPSILON);
       inputRandFltEpsilon_int[i] = sign?1:0;
   }

   for( i=0; i < sizeof(inputRandFltEpsilon)/sizeof(double); i++ )
   {
       sign= (unsigned int)rand()%2;
       inputRandFltEpsilon[i] = (sign?1.0:-1.0)*(DBL_EPSILON);
       inputRandFltEpsilon_int[i] = sign?1:-1;
   }

   TA_ForEachFunc( testDefault, &errNumber );
   return errNumber;
}
