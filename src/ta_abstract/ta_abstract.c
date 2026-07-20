/* TA-LIB Copyright (c) 1999-2026, Mario Fortier
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

/* Description:
 *   Provide a way to abstract the call of the TA functions.
 */

#include <stddef.h>
#include <string.h>
#include <ctype.h>
#include "ta_common.h"
#include "ta_memory.h"
#include "ta_abstract.h"
#include "ta_def_ui.h"
#include "ta_frame_priv.h"

#include <limits.h>

extern const TA_FuncDef *TA_DEF_TableA[];
extern const TA_FuncDef *TA_DEF_TableB[];
extern const TA_FuncDef *TA_DEF_TableC[];
extern const TA_FuncDef *TA_DEF_TableD[];
extern const TA_FuncDef *TA_DEF_TableE[];
extern const TA_FuncDef *TA_DEF_TableF[];
extern const TA_FuncDef *TA_DEF_TableG[];
extern const TA_FuncDef *TA_DEF_TableH[];
extern const TA_FuncDef *TA_DEF_TableI[];
extern const TA_FuncDef *TA_DEF_TableJ[];
extern const TA_FuncDef *TA_DEF_TableK[];
extern const TA_FuncDef *TA_DEF_TableL[];
extern const TA_FuncDef *TA_DEF_TableM[];
extern const TA_FuncDef *TA_DEF_TableN[];
extern const TA_FuncDef *TA_DEF_TableO[];
extern const TA_FuncDef *TA_DEF_TableP[];
extern const TA_FuncDef *TA_DEF_TableQ[];
extern const TA_FuncDef *TA_DEF_TableR[];
extern const TA_FuncDef *TA_DEF_TableS[];
extern const TA_FuncDef *TA_DEF_TableT[];
extern const TA_FuncDef *TA_DEF_TableU[];
extern const TA_FuncDef *TA_DEF_TableV[];
extern const TA_FuncDef *TA_DEF_TableW[];
extern const TA_FuncDef *TA_DEF_TableX[];
extern const TA_FuncDef *TA_DEF_TableY[];
extern const TA_FuncDef *TA_DEF_TableZ[];

extern const unsigned int TA_DEF_TableASize, TA_DEF_TableBSize,
                          TA_DEF_TableCSize, TA_DEF_TableDSize,
                          TA_DEF_TableESize, TA_DEF_TableFSize,
                          TA_DEF_TableGSize, TA_DEF_TableHSize,
                          TA_DEF_TableISize, TA_DEF_TableJSize,
                          TA_DEF_TableKSize, TA_DEF_TableLSize,
                          TA_DEF_TableMSize, TA_DEF_TableNSize,
                          TA_DEF_TableOSize, TA_DEF_TablePSize,
                          TA_DEF_TableQSize, TA_DEF_TableRSize,
                          TA_DEF_TableSSize, TA_DEF_TableTSize,
                          TA_DEF_TableUSize, TA_DEF_TableVSize,
                          TA_DEF_TableWSize, TA_DEF_TableXSize,
                          TA_DEF_TableYSize, TA_DEF_TableZSize;

extern const TA_FuncDef **TA_PerGroupFuncDef[];
extern const unsigned int TA_PerGroupSize[];

#ifndef min
   #define min(a, b)  (((a) < (b)) ? (a) : (b))
#endif

typedef struct
{
   unsigned int magicNumber;
} TA_StringTablePriv;

static TA_RetCode getGroupId( const char *groupString, unsigned int *groupId );
static TA_RetCode getGroupSize( TA_GroupId groupId, unsigned int *groupSize );
static TA_RetCode getFuncNameByIdx( TA_GroupId groupId,
                                     unsigned int idx,
                                     const char **stringPtr );

static const TA_FuncDef **TA_DEF_Tables[26] =
{
   (const TA_FuncDef **)TA_DEF_TableA,    (const TA_FuncDef **)TA_DEF_TableB,    (const TA_FuncDef **)TA_DEF_TableC,    (const TA_FuncDef **)TA_DEF_TableD,    (const TA_FuncDef **)TA_DEF_TableE,
   (const TA_FuncDef **)TA_DEF_TableF,    (const TA_FuncDef **)TA_DEF_TableG,    (const TA_FuncDef **)TA_DEF_TableH,    (const TA_FuncDef **)TA_DEF_TableI,    (const TA_FuncDef **)TA_DEF_TableJ,
   (const TA_FuncDef **)TA_DEF_TableK,    (const TA_FuncDef **)TA_DEF_TableL,    (const TA_FuncDef **)TA_DEF_TableM,    (const TA_FuncDef **)TA_DEF_TableN,    (const TA_FuncDef **)TA_DEF_TableO,
   (const TA_FuncDef **)TA_DEF_TableP,    (const TA_FuncDef **)TA_DEF_TableQ,    (const TA_FuncDef **)TA_DEF_TableR,    (const TA_FuncDef **)TA_DEF_TableS,    (const TA_FuncDef **)TA_DEF_TableT,
   (const TA_FuncDef **)TA_DEF_TableU,    (const TA_FuncDef **)TA_DEF_TableV,    (const TA_FuncDef **)TA_DEF_TableW,    (const TA_FuncDef **)TA_DEF_TableX,    (const TA_FuncDef **)TA_DEF_TableY,
   (const TA_FuncDef **)TA_DEF_TableZ };

static const unsigned int *TA_DEF_TablesSize[26] =
{
   &TA_DEF_TableASize,    &TA_DEF_TableBSize,    &TA_DEF_TableCSize,
   &TA_DEF_TableDSize,    &TA_DEF_TableESize,    &TA_DEF_TableFSize,
   &TA_DEF_TableGSize,    &TA_DEF_TableHSize,    &TA_DEF_TableISize,
   &TA_DEF_TableJSize,    &TA_DEF_TableKSize,    &TA_DEF_TableLSize,
   &TA_DEF_TableMSize,    &TA_DEF_TableNSize,    &TA_DEF_TableOSize,
   &TA_DEF_TablePSize,    &TA_DEF_TableQSize,    &TA_DEF_TableRSize,
   &TA_DEF_TableSSize,    &TA_DEF_TableTSize,    &TA_DEF_TableUSize,
   &TA_DEF_TableVSize,    &TA_DEF_TableWSize,    &TA_DEF_TableXSize,
   &TA_DEF_TableYSize,    &TA_DEF_TableZSize };

TA_RetCode TA_GroupTableAlloc( TA_StringTable **table )
{
   TA_StringTable *stringTable;
   TA_StringTablePriv *stringTablePriv;

   if( table == NULL )
   {
      return TA_BAD_PARAM;
   }

   stringTable = (TA_StringTable *)TA_Malloc( sizeof(TA_StringTable) + sizeof(TA_StringTablePriv) );
   if( !stringTable )
   {
      *table = NULL;
      return TA_ALLOC_ERR;
   }

   memset( stringTable, 0, sizeof(TA_StringTable) + sizeof(TA_StringTablePriv) );
   stringTablePriv = (TA_StringTablePriv *)(((char *)stringTable)+sizeof(TA_StringTable));
   stringTablePriv->magicNumber = TA_STRING_TABLE_GROUP_MAGIC_NB;

   stringTable->size = TA_NB_GROUP_ID;
   stringTable->string = &TA_GroupString[0];
   stringTable->hiddenData = stringTablePriv;

   *table = stringTable;

   return TA_SUCCESS;
}

TA_RetCode TA_GroupTableFree( TA_StringTable *table )
{
   TA_StringTablePriv *stringTablePriv;

   if( table )
   {
      stringTablePriv = (TA_StringTablePriv *)table->hiddenData;
      if( !stringTablePriv )
      {
         return TA_INTERNAL_ERROR(1);
      }

      if( stringTablePriv->magicNumber != TA_STRING_TABLE_GROUP_MAGIC_NB )
      {
         return TA_BAD_OBJECT;
      }

      TA_Free( table );
   }

   return TA_SUCCESS;
}

TA_RetCode TA_ForEachFunc( TA_CallForEachFunc functionToCall, void *opaqueData )
{
   const TA_FuncDef **funcDefTable;
   const TA_FuncDef *funcDef;
   const TA_FuncInfo *funcInfo;
   unsigned int i, j, funcDefTableSize;

   if( functionToCall == NULL )
   {
      return TA_BAD_PARAM;
   }

   for( i=0; i < 26; i++ )
   {
      funcDefTable = TA_DEF_Tables[i];
      funcDefTableSize = *TA_DEF_TablesSize[i];
      for( j=0; j < funcDefTableSize; j++ )
      {
         funcDef = funcDefTable[j];
         if( !funcDef || !funcDef->funcInfo )
            return TA_INTERNAL_ERROR(3);

         funcInfo = funcDef->funcInfo;
         if( !funcInfo )
            return TA_INTERNAL_ERROR(4);
         (*functionToCall)( funcInfo, opaqueData );
      }
   }

   return TA_SUCCESS;
}

TA_RetCode TA_FuncTableAlloc( const char *group, TA_StringTable **table )
{
   TA_RetCode retCode;
   unsigned int i;
   TA_StringTable *stringTable;
   unsigned int groupId;
   unsigned int groupSize;
   const char *stringPtr;
   TA_StringTablePriv *stringTablePriv;

   if( (group == NULL) || (table == NULL ) )
   {
      return TA_BAD_PARAM;
   }

   *table = NULL;
   stringPtr = NULL;

   retCode = getGroupId( group, &groupId );
   if( retCode != TA_SUCCESS )
   {
      return retCode;
   }

   retCode = getGroupSize( (TA_GroupId)groupId, &groupSize );
   if( retCode != TA_SUCCESS )
   {
      return retCode;
   }

   stringTable = (TA_StringTable *)TA_Malloc( sizeof(TA_StringTable) + sizeof(TA_StringTablePriv) );
   if( !stringTable )
   {
      *table = NULL;
      return TA_ALLOC_ERR;
   }

   memset( stringTable, 0, sizeof(TA_StringTable) + sizeof(TA_StringTablePriv) );
   stringTablePriv = (TA_StringTablePriv *)(((char *)stringTable)+sizeof(TA_StringTable));
   stringTablePriv->magicNumber = TA_STRING_TABLE_FUNC_MAGIC_NB;
   stringTable->hiddenData = stringTablePriv;

   stringTable->size = groupSize;
   if( groupSize != 0 )
   {
      stringTable->string = (const char **)TA_Malloc( (stringTable->size) *
                                                      sizeof(const char *) );

      if( stringTable->string == NULL )
      {
         *table = NULL;
         TA_FuncTableFree( stringTable );
         return TA_ALLOC_ERR;
      }

      memset( (void *)stringTable->string, 0,
              (stringTable->size) * sizeof(const char *) );

      for( i=0; i < stringTable->size; i++ )
      {
         retCode = getFuncNameByIdx( (TA_GroupId)groupId, i, &stringPtr );

         if( retCode != TA_SUCCESS )
         {
            *table = NULL;
            TA_FuncTableFree( stringTable );
            return TA_ALLOC_ERR;
         }

         (stringTable->string)[i] = stringPtr;
      }
   }

   *table = stringTable;

   return TA_SUCCESS;
}

TA_RetCode TA_FuncTableFree( TA_StringTable *table )
{
   TA_StringTablePriv *stringTablePriv;

   if( table )
   {
      stringTablePriv = (TA_StringTablePriv *)table->hiddenData;
      if( !stringTablePriv )
      {
         return TA_INTERNAL_ERROR(3);
      }

      if( stringTablePriv->magicNumber != TA_STRING_TABLE_FUNC_MAGIC_NB )
      {
         return TA_BAD_OBJECT;
      }

      if( table->string )
         TA_Free( (void *)table->string );

      TA_Free( table );
   }

   return TA_SUCCESS;
}

TA_RetCode TA_GetFuncHandle( const char *name, const TA_FuncHandle **handle )
{
   char firstChar, tmp;
   const TA_FuncDef **funcDefTable;
   const TA_FuncDef *funcDef;
   const TA_FuncInfo *funcInfo;
   unsigned int i, funcDefTableSize;

   if( (name == NULL) || (handle == NULL) )
   {
      return TA_BAD_PARAM;
   }

   *handle = NULL;

   firstChar = name[0];

   if( firstChar == '\0' )
   {
      return TA_BAD_PARAM;
   }

   tmp = (char)tolower( firstChar );

   if( (tmp < 'a') || (tmp > 'z') )
   {
      return TA_FUNC_NOT_FOUND;
   }

   tmp -= (char)'a';
   funcDefTable = TA_DEF_Tables[(int)tmp];

   funcDefTableSize = *TA_DEF_TablesSize[(int)tmp];
   if( funcDefTableSize < 1 )
   {
      return TA_FUNC_NOT_FOUND;
   }

   for( i=0; i < funcDefTableSize; i++ )
   {
      funcDef = funcDefTable[i];
      if( !funcDef || !funcDef->funcInfo )
         return TA_INTERNAL_ERROR(3);

      funcInfo = funcDef->funcInfo;
      if( !funcInfo )
         return TA_INTERNAL_ERROR(4);

      if( strcmp( funcInfo->name, name ) == 0 )
      {
         *handle = (TA_FuncHandle *)funcDef;
         return TA_SUCCESS;
      }
   }

   return TA_FUNC_NOT_FOUND;
}

TA_RetCode TA_GetFuncInfo(  const TA_FuncHandle *handle,
                            const TA_FuncInfo **funcInfo )
{
   const TA_FuncDef *funcDef;

   if( !funcInfo || !handle )
   {
      return TA_BAD_PARAM;
   }

   funcDef = (const TA_FuncDef *)handle;
   if( funcDef->magicNumber != TA_FUNC_DEF_MAGIC_NB )
   {
      return TA_INVALID_HANDLE;
   }

   *funcInfo = funcDef->funcInfo;
   if( !funcDef->funcInfo )
      return TA_INVALID_HANDLE;

   return TA_SUCCESS;
}

TA_RetCode TA_GetInputParameterInfo( const TA_FuncHandle *handle,
                                     unsigned int paramIndex,
                                     const TA_InputParameterInfo **info )
{
   const TA_FuncDef  *funcDef;
   const TA_FuncInfo *funcInfo;
   const TA_InputParameterInfo **inputTable;

   if( (handle == NULL) || (info == NULL) )
   {
      return TA_BAD_PARAM;
   }

   *info = NULL;

   funcDef = (const TA_FuncDef *)handle;
   if( funcDef->magicNumber != TA_FUNC_DEF_MAGIC_NB )
   {
      return TA_INVALID_HANDLE;
   }
   funcInfo = funcDef->funcInfo;

   if( !funcInfo ) return TA_INVALID_HANDLE;

   if( paramIndex >= funcInfo->nbInput )
   {
      return TA_BAD_PARAM;
   }

   inputTable = (const TA_InputParameterInfo **)funcDef->input;

   if( !inputTable )
      return TA_INTERNAL_ERROR(2);

   *info = inputTable[paramIndex];

   if( !(*info) )
      return TA_INTERNAL_ERROR(3);

   return TA_SUCCESS;
}

TA_RetCode TA_GetOptInputParameterInfo( const TA_FuncHandle *handle,
                                        unsigned int paramIndex,
                                        const TA_OptInputParameterInfo **info )
{
   const TA_FuncDef  *funcDef;
   const TA_FuncInfo *funcInfo;
   const TA_OptInputParameterInfo **inputTable;

   if( (handle == NULL) || (info == NULL) )
   {
      return TA_BAD_PARAM;
   }

   *info = NULL;

   funcDef = (const TA_FuncDef *)handle;
   if( funcDef->magicNumber != TA_FUNC_DEF_MAGIC_NB )
   {
      return TA_INVALID_HANDLE;
   }

   funcInfo = funcDef->funcInfo;

   if( !funcInfo )
      return TA_INVALID_HANDLE;

   if( paramIndex >= funcInfo->nbOptInput )
   {
      return TA_BAD_PARAM;
   }

   inputTable = (const TA_OptInputParameterInfo **)funcDef->optInput;

   if( !inputTable )
      return TA_INTERNAL_ERROR(3);

   *info = inputTable[paramIndex];

   if( !(*info) )
      return TA_INTERNAL_ERROR(4);

   return TA_SUCCESS;
}

TA_RetCode TA_GetOutputParameterInfo( const TA_FuncHandle *handle,
                                      unsigned int paramIndex,
                                      const TA_OutputParameterInfo **info )
{

   const TA_FuncDef  *funcDef;
   const TA_FuncInfo *funcInfo;
   const TA_OutputParameterInfo **outputTable;

   if( (handle == NULL) || (info == NULL) )
   {
      return TA_BAD_PARAM;
   }

   *info = NULL;

   funcDef = (const TA_FuncDef *)handle;
   if( funcDef->magicNumber != TA_FUNC_DEF_MAGIC_NB )
   {
      return TA_INVALID_HANDLE;
   }

   funcInfo = funcDef->funcInfo;

   if( !funcInfo )
   {
      return TA_INVALID_HANDLE;
   }

   if( paramIndex >= funcInfo->nbOutput )
   {
      return TA_BAD_PARAM;
   }

   outputTable = (const TA_OutputParameterInfo **)funcDef->output;

   if( !outputTable )
   {
      return TA_INTERNAL_ERROR(4);
   }

   *info = outputTable[paramIndex];

   if( !(*info) )
   {
      return TA_INTERNAL_ERROR(5);
   }

   return TA_SUCCESS;
}

TA_RetCode TA_ParamHolderAlloc( const TA_FuncHandle *handle,
                                TA_ParamHolder **allocatedParams )
{

   TA_FuncDef *funcDef;
   unsigned int allocSize, i;
   TA_ParamHolderInput    *input;
   TA_ParamHolderOptInput *optInput;
   TA_ParamHolderOutput   *output;

   const TA_FuncInfo *funcInfo;
   TA_ParamHolder *newParams;
   TA_ParamHolderPriv *newParamsPriv;

   const TA_InputParameterInfo    **inputInfo;
   const TA_OptInputParameterInfo **optInputInfo;
   const TA_OutputParameterInfo   **outputInfo;

   if( !handle || !allocatedParams)
   {
      return TA_BAD_PARAM;
   }

   funcDef = (TA_FuncDef *)handle;
   if( funcDef->magicNumber != TA_FUNC_DEF_MAGIC_NB )
   {
      *allocatedParams = NULL;
      return TA_INVALID_HANDLE;
   }

   funcInfo = funcDef->funcInfo;
   if( !funcInfo ) return TA_INVALID_HANDLE;

   newParams = (TA_ParamHolder *)TA_Malloc( sizeof(TA_ParamHolder) + sizeof(TA_ParamHolderPriv));
   if( !newParams )
   {
      *allocatedParams = NULL;
      return TA_ALLOC_ERR;
   }

   memset( newParams, 0, sizeof(TA_ParamHolder) + sizeof(TA_ParamHolderPriv) );
   newParamsPriv = (TA_ParamHolderPriv *)(((char *)newParams)+sizeof(TA_ParamHolder));
   newParamsPriv->magicNumber = TA_PARAM_HOLDER_PRIV_MAGIC_NB;
   newParams->hiddenData = newParamsPriv;

   if( funcInfo->nbInput == 0 ) return TA_INTERNAL_ERROR(2);

   allocSize = (funcInfo->nbInput) * sizeof(TA_ParamHolderInput);
   input = (TA_ParamHolderInput *)TA_Malloc( allocSize );

   if( !input )
   {
      TA_ParamHolderFree( newParams );
      *allocatedParams = NULL;
      return TA_ALLOC_ERR;
   }
   memset( input, 0, allocSize );
   newParamsPriv->in = input;

   if( funcInfo->nbOptInput == 0 )
      optInput = NULL;
   else
   {
      allocSize = (funcInfo->nbOptInput) * sizeof(TA_ParamHolderOptInput);
      optInput = (TA_ParamHolderOptInput *)TA_Malloc( allocSize );

      if( !optInput )
      {
         TA_ParamHolderFree( newParams );
         *allocatedParams = NULL;
         return TA_ALLOC_ERR;
      }
      memset( optInput, 0, allocSize );
   }
   newParamsPriv->optIn = optInput;

   allocSize = (funcInfo->nbOutput) * sizeof(TA_ParamHolderOutput);
   output = (TA_ParamHolderOutput *)TA_Malloc( allocSize );
   if( !output )
   {
      TA_ParamHolderFree( newParams );
      *allocatedParams = NULL;
      return TA_ALLOC_ERR;
   }
   memset( output, 0, allocSize );
   newParamsPriv->out = output;

   newParamsPriv->funcInfo = funcInfo;

   inputInfo    = (const TA_InputParameterInfo **)funcDef->input;
   optInputInfo = (const TA_OptInputParameterInfo **)funcDef->optInput;
   outputInfo   = (const TA_OutputParameterInfo   **)funcDef->output;

   for( i=0; i < funcInfo->nbInput; i++ )
   {
      input[i].inputInfo = inputInfo[i];
      newParamsPriv->inBitmap <<= 1;
      newParamsPriv->inBitmap |= 1;
   }

   for( i=0; i < funcInfo->nbOptInput; i++ )
   {
      optInput[i].optInputInfo = optInputInfo[i];
      if( optInput[i].optInputInfo->type == TA_OptInput_RealRange )
         optInput[i].data.optInReal = optInputInfo[i]->defaultValue;
      else
         optInput[i].data.optInInteger = (TA_Integer)optInputInfo[i]->defaultValue;
   }

   for( i=0; i < funcInfo->nbOutput; i++ )
   {
      output[i].outputInfo = outputInfo[i];
      newParamsPriv->outBitmap <<= 1;
      newParamsPriv->outBitmap |= 1;
   }

   *allocatedParams = newParams;

   return TA_SUCCESS;
}

TA_RetCode TA_ParamHolderFree( TA_ParamHolder *paramsToFree )
{
   TA_ParamHolderPriv     *paramPriv;

   TA_ParamHolderInput    *input;
   TA_ParamHolderOptInput *optInput;
   TA_ParamHolderOutput   *output;

   if( !paramsToFree )
   {
      return TA_SUCCESS;
   }

   paramPriv = paramsToFree->hiddenData;

   if( !paramPriv )
   {
      return TA_INVALID_PARAM_HOLDER;
   }

   if( paramPriv->magicNumber != TA_PARAM_HOLDER_PRIV_MAGIC_NB )
   {
      return TA_INVALID_PARAM_HOLDER;
   }

   optInput = paramPriv->optIn;
   if( optInput )
      TA_Free( optInput );

   input = paramPriv->in;
   if( input )
      TA_Free( input );

   output = paramPriv->out;
   if( output )
      TA_Free( output );

   TA_Free( paramsToFree );

   return TA_SUCCESS;
}

TA_RetCode TA_SetInputParamIntegerPtr( TA_ParamHolder *param,
                                       unsigned int paramIndex,
                                       const TA_Integer *value )
{

   TA_ParamHolderPriv *paramHolderPriv;
   const TA_InputParameterInfo *paramInfo;
   const TA_FuncInfo *funcInfo;

   if( (param == NULL) || (value == NULL) )
   {
      return TA_BAD_PARAM;
   }

   paramHolderPriv = (TA_ParamHolderPriv *)(param->hiddenData);
   if( paramHolderPriv->magicNumber != TA_PARAM_HOLDER_PRIV_MAGIC_NB )
   {
      return TA_INVALID_PARAM_HOLDER;
   }

   funcInfo = paramHolderPriv->funcInfo;
   if( !funcInfo ) return TA_INVALID_HANDLE;

   if( paramIndex >= funcInfo->nbInput )
   {
      return TA_BAD_PARAM;
   }

   paramInfo = paramHolderPriv->in[paramIndex].inputInfo;
   if( !paramInfo ) return TA_INTERNAL_ERROR(2);
   if( paramInfo->type != TA_Input_Integer )
   {
      return TA_INVALID_PARAM_HOLDER_TYPE;
   }

   paramHolderPriv->in[paramIndex].data.inInteger = value;

   paramHolderPriv->inBitmap &= ~(1<<paramIndex);

   return TA_SUCCESS;
}

TA_RetCode TA_SetInputParamRealPtr( TA_ParamHolder *param,
                                    unsigned int paramIndex,
                                    const TA_Real *value )
{
   TA_ParamHolderPriv *paramHolderPriv;
   const TA_InputParameterInfo *paramInfo;
   const TA_FuncInfo *funcInfo;

   if( (param == NULL) || (value == NULL) )
   {
      return TA_BAD_PARAM;
   }

   paramHolderPriv = (TA_ParamHolderPriv *)(param->hiddenData);
   if( paramHolderPriv->magicNumber != TA_PARAM_HOLDER_PRIV_MAGIC_NB )
   {
      return TA_INVALID_PARAM_HOLDER;
   }

   funcInfo = paramHolderPriv->funcInfo;
   if( !funcInfo ) return TA_INVALID_HANDLE;

   if( paramIndex >= funcInfo->nbInput )
   {
      return TA_BAD_PARAM;
   }

   paramInfo = paramHolderPriv->in[paramIndex].inputInfo;
   if( !paramInfo ) return TA_INTERNAL_ERROR(2);
   if( paramInfo->type != TA_Input_Real )
   {
      return TA_INVALID_PARAM_HOLDER_TYPE;
   }

   paramHolderPriv->in[paramIndex].data.inReal = value;

   paramHolderPriv->inBitmap &= ~(1<<paramIndex);

   return TA_SUCCESS;
}

TA_RetCode TA_SetInputParamPricePtr( TA_ParamHolder     *param,
                                     unsigned int        paramIndex,
                                     const TA_Real      *open,
                                     const TA_Real      *high,
                                     const TA_Real      *low,
                                     const TA_Real      *close,
                                     const TA_Real      *volume,
                                     const TA_Real      *openInterest )
{

   TA_ParamHolderPriv *paramHolderPriv;
   const TA_InputParameterInfo *paramInfo;
   const TA_FuncInfo *funcInfo;

   if( param == NULL )
   {
      return TA_BAD_PARAM;
   }

   paramHolderPriv = (TA_ParamHolderPriv *)(param->hiddenData);
   if( paramHolderPriv->magicNumber != TA_PARAM_HOLDER_PRIV_MAGIC_NB )
   {
      return TA_INVALID_PARAM_HOLDER;
   }

   funcInfo = paramHolderPriv->funcInfo;
   if( !funcInfo ) return TA_INVALID_HANDLE;
   if( paramIndex >= funcInfo->nbInput )
   {
      return TA_BAD_PARAM;
   }

   paramInfo = paramHolderPriv->in[paramIndex].inputInfo;
   if( !paramInfo ) return TA_INTERNAL_ERROR(2);
   if( paramInfo->type != TA_Input_Price )
   {
      return TA_INVALID_PARAM_HOLDER_TYPE;
   }

   #define SET_PARAM_INFO(lowerParam,upperParam) \
   { \
      if( paramInfo->flags & TA_IN_PRICE_##upperParam ) \
      { \
         if( lowerParam == NULL ) \
         { \
            return TA_BAD_PARAM; \
         } \
         paramHolderPriv->in[paramIndex].data.inPrice.lowerParam = lowerParam; \
      } \
   }

   SET_PARAM_INFO(open, OPEN );
   SET_PARAM_INFO(high, HIGH );
   SET_PARAM_INFO(low, LOW );
   SET_PARAM_INFO(close, CLOSE );
   SET_PARAM_INFO(volume, VOLUME );
   SET_PARAM_INFO(openInterest, OPENINTEREST );

   #undef SET_PARAM_INFO

   paramHolderPriv->inBitmap &= ~(1<<paramIndex);

   return TA_SUCCESS;
}

TA_RetCode TA_SetOptInputParamInteger( TA_ParamHolder *param,
                                       unsigned int paramIndex,
                                       TA_Integer value )
{

   TA_ParamHolderPriv *paramHolderPriv;
   const TA_OptInputParameterInfo *paramInfo;
   const TA_FuncInfo *funcInfo;

   if( param == NULL )
   {
      return TA_BAD_PARAM;
   }

   paramHolderPriv = (TA_ParamHolderPriv *)(param->hiddenData);
   if( paramHolderPriv->magicNumber != TA_PARAM_HOLDER_PRIV_MAGIC_NB )
   {
      return TA_INVALID_PARAM_HOLDER;
   }

   funcInfo = paramHolderPriv->funcInfo;
   if( !funcInfo ) return TA_INVALID_HANDLE;
   if( paramIndex >= funcInfo->nbOptInput )
   {
      return TA_BAD_PARAM;
   }

   paramInfo = paramHolderPriv->optIn[paramIndex].optInputInfo;
   if( !paramInfo ) return TA_INTERNAL_ERROR(2);
   if( (paramInfo->type != TA_OptInput_IntegerRange) &&
       (paramInfo->type != TA_OptInput_IntegerList) )
   {
      return TA_INVALID_PARAM_HOLDER_TYPE;
   }

   paramHolderPriv->optIn[paramIndex].data.optInInteger = value;

   return TA_SUCCESS;
}

TA_RetCode TA_SetOptInputParamReal( TA_ParamHolder *param,
                                    unsigned int paramIndex,
                                    TA_Real value )
{
   TA_ParamHolderPriv *paramHolderPriv;
   const TA_OptInputParameterInfo *paramInfo;
   const TA_FuncInfo *funcInfo;

   if( param == NULL )
   {
      return TA_BAD_PARAM;
   }

   paramHolderPriv = (TA_ParamHolderPriv *)(param->hiddenData);
   if( paramHolderPriv->magicNumber != TA_PARAM_HOLDER_PRIV_MAGIC_NB )
   {
      return TA_INVALID_PARAM_HOLDER;
   }

   funcInfo = paramHolderPriv->funcInfo;
   if( !funcInfo ) return TA_INVALID_HANDLE;

   if( paramIndex >= funcInfo->nbOptInput )
   {
      return TA_BAD_PARAM;
   }

   paramInfo = paramHolderPriv->optIn[paramIndex].optInputInfo;
   if( !paramInfo ) return TA_INTERNAL_ERROR(2);
   if( (paramInfo->type != TA_OptInput_RealRange) &&
       (paramInfo->type != TA_OptInput_RealList) )
   {
      return TA_INVALID_PARAM_HOLDER_TYPE;
   }

   paramHolderPriv->optIn[paramIndex].data.optInReal = value;

   return TA_SUCCESS;
}

TA_RetCode TA_SetOutputParamIntegerPtr( TA_ParamHolder *param,
                                        unsigned int paramIndex,
                                        TA_Integer     *out )
{
   TA_ParamHolderPriv *paramHolderPriv;
   const TA_OutputParameterInfo *paramInfo;
   const TA_FuncInfo *funcInfo;

   if( (param == NULL) || (out == NULL) )
   {
      return TA_BAD_PARAM;
   }

   paramHolderPriv = (TA_ParamHolderPriv *)(param->hiddenData);
   if( paramHolderPriv->magicNumber != TA_PARAM_HOLDER_PRIV_MAGIC_NB )
   {
      return TA_INVALID_PARAM_HOLDER;
   }

   funcInfo = paramHolderPriv->funcInfo;
   if( !funcInfo ) return TA_INVALID_HANDLE;

   if( paramIndex >= funcInfo->nbOutput )
   {
      return TA_BAD_PARAM;
   }

   paramInfo = paramHolderPriv->out[paramIndex].outputInfo;
   if( !paramInfo ) return TA_INTERNAL_ERROR(2);
   if( paramInfo->type != TA_Output_Integer )
   {
      return TA_INVALID_PARAM_HOLDER_TYPE;
   }

   paramHolderPriv->out[paramIndex].data.outInteger = out;

   paramHolderPriv->outBitmap &= ~(1<<paramIndex);

   return TA_SUCCESS;
}

TA_RetCode TA_SetOutputParamRealPtr( TA_ParamHolder *param,
                                     unsigned int paramIndex,
                                     TA_Real        *out )
{
   TA_ParamHolderPriv *paramHolderPriv;
   const TA_OutputParameterInfo *paramInfo;
   const TA_FuncInfo *funcInfo;

   if( (param == NULL) || (out == NULL) )
   {
      return TA_BAD_PARAM;
   }

   paramHolderPriv = (TA_ParamHolderPriv *)(param->hiddenData);
   if( paramHolderPriv->magicNumber != TA_PARAM_HOLDER_PRIV_MAGIC_NB )
   {
      return TA_INVALID_PARAM_HOLDER;
   }

   funcInfo = paramHolderPriv->funcInfo;
   if( !funcInfo ) return TA_INVALID_HANDLE;

   if( paramIndex >= funcInfo->nbOutput )
   {
      return TA_BAD_PARAM;
   }

   paramInfo = paramHolderPriv->out[paramIndex].outputInfo;
   if( !paramInfo ) return TA_INTERNAL_ERROR(2);
   if( paramInfo->type != TA_Output_Real )
   {
      return TA_INVALID_PARAM_HOLDER_TYPE;
   }

   paramHolderPriv->out[paramIndex].data.outReal = out;

   paramHolderPriv->outBitmap &= ~(1<<paramIndex);

   return TA_SUCCESS;
}

TA_RetCode TA_GetLookback( const TA_ParamHolder *param, TA_Integer *lookback )
{
   const TA_ParamHolderPriv *paramHolderPriv;

   const TA_FuncDef *funcDef;
   const TA_FuncInfo *funcInfo;
   TA_FrameLookback lookbackFunction;

   if( (param == NULL) || (lookback == NULL))
   {
      return TA_BAD_PARAM;
   }

   paramHolderPriv = (TA_ParamHolderPriv *)(param->hiddenData);
   if( paramHolderPriv->magicNumber != TA_PARAM_HOLDER_PRIV_MAGIC_NB )
   {
      return TA_INVALID_PARAM_HOLDER;
   }

   funcInfo = paramHolderPriv->funcInfo;
   if( !funcInfo ) return TA_INVALID_HANDLE;

   funcDef = (const TA_FuncDef *)funcInfo->handle;
   if( !funcDef ) return TA_INTERNAL_ERROR(2);
   lookbackFunction = funcDef->lookback;
   if( !lookbackFunction ) return TA_INTERNAL_ERROR(2);

   *lookback = (*lookbackFunction)( paramHolderPriv );

   return TA_SUCCESS;
}

TA_RetCode TA_CallFunc( const TA_ParamHolder *param,
                        TA_Integer            startIdx,
                        TA_Integer            endIdx,
                        TA_Integer           *outBegIdx,
                        TA_Integer           *outNbElement )
{
   TA_RetCode retCode;
   const TA_ParamHolderPriv *paramHolderPriv;

   const TA_FuncDef *funcDef;
   const TA_FuncInfo *funcInfo;
   TA_FrameFunction function;

   if( (param == NULL) ||
       (outBegIdx == NULL) ||
       (outNbElement == NULL) )
   {
      return TA_BAD_PARAM;
   }

   paramHolderPriv = (TA_ParamHolderPriv *)(param->hiddenData);
   if( paramHolderPriv->magicNumber != TA_PARAM_HOLDER_PRIV_MAGIC_NB )
   {
      return TA_INVALID_PARAM_HOLDER;
   }

   if( paramHolderPriv->inBitmap != 0 )
   {
      return TA_INPUT_NOT_ALL_INITIALIZE;
   }

   if( paramHolderPriv->outBitmap != 0 )
   {
      return TA_OUTPUT_NOT_ALL_INITIALIZE;
   }

   funcInfo = paramHolderPriv->funcInfo;
   if( !funcInfo ) return TA_INVALID_HANDLE;
   funcDef = (const TA_FuncDef *)funcInfo->handle;
   if( !funcDef ) return TA_INTERNAL_ERROR(2);
   function = funcDef->function;
   if( !function ) return TA_INTERNAL_ERROR(2);

   retCode = (*function)( paramHolderPriv, startIdx, endIdx,
                          outBegIdx, outNbElement );
   return retCode;
}

static TA_RetCode getGroupId( const char *groupString, unsigned int *groupId )
{
   unsigned int i;

   for( i=0; i < TA_NB_GROUP_ID; i++ )
   {
      if( strcmp( TA_GroupString[i], groupString ) == 0 )
      {
         *groupId = i;
         return TA_SUCCESS;
      }
   }

   return TA_GROUP_NOT_FOUND;
}

static TA_RetCode getGroupSize( TA_GroupId groupId, unsigned int *groupSize )
{
   *groupSize = TA_PerGroupSize[groupId];

   return TA_SUCCESS;
}

static TA_RetCode getFuncNameByIdx( TA_GroupId groupId,
                                     unsigned int idx,
                                     const char **stringPtr )
{
   const TA_FuncDef **funcDefTable;
   const TA_FuncInfo *funcInfo;

   funcDefTable = TA_PerGroupFuncDef[groupId];
   funcInfo = funcDefTable[idx]->funcInfo;
   *stringPtr = funcInfo->name;

   return TA_SUCCESS;
}
