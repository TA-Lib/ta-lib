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

/**** Local variables definitions.     ****/
/* None */

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


