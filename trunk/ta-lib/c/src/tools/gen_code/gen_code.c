/* 
 * This utility may need some clean-up. It has been quickly written for
 * helping to the automatisation of repetitive task for the TA-LIB
 * developpers. 
 *
 * This utility have no used for an end-user of the TA-LIB.
 * It is useful only to people integrating TA function in
 * TA-Lib.
 *
 * Note: All directory in this code is relative to the 'bin'
 *       directory. So you must run the executable from ta-lib/c/bin.
 *       
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ta_common.h"
#include "ta_abstract.h"
#include "ta_system.h"
#include "sfl.h"

#define FILE_READ  1
#define FILE_WRITE 0

#ifndef min
   #define min(a, b)  (((a) < (b)) ? (a) : (b))
#endif

#ifndef max
   #define max(a, b)  (((a) > (b)) ? (a) : (b))
#endif

typedef struct
{
   FILE *file;
   FILE *templateFile;
} FileHandle;

FileHandle *gOutFunc_H;        /* For "ta_func.h"  */
FileHandle *gOutFrame_H;       /* For "ta_frame.h" */
FileHandle *gOutFrame_C;       /* For "ta_frame.c" */
FileHandle *gOutGroupIdx_C;    /* For "ta_group_idx.c" */
FileHandle *gOutFunc_C;        /* For "ta_x.c" where 'x' is TA function name. */
FileHandle *gOutRetCode_C;     /* For "ta_retcode.c" */
FileHandle *gOutRetCode_CSV;   /* For "ta_retcode.csv" */
FileHandle *gOutFuncList_TXT;  /* For "func_list.txt" */
FileHandle *gOutExcelGlue_C;   /* For "excel_glue.c" */
FileHandle *gOutDefs_H;        /* for "ta_defs.h" */

typedef void (*TA_ForEachGroup)( const char *groupName,
                                 unsigned int index,
                                 unsigned int isFirst, /* Boolean */
                                 unsigned int isLast   /* Boolean */
                                );

static unsigned int forEachGroup( TA_ForEachGroup forEachGroupfunc,
                                  void *opaqueData );

static void doForEachFunction( const TA_FuncInfo *funcInfo,
                               void *opaqueData );

static void doForEachUnstableFunction( const TA_FuncInfo *funcInfo,
                                       void *opaqueData );

static void doDefsFile( void );

static int gen_retcode( void );

static void printIndent( FILE *out, unsigned int indent );

static void printFunc( FILE *out,
                       const char *prefix,       /* Can be NULL */
                       const TA_FuncInfo *funcInfo,
                       unsigned int prototype,         /* Boolean */
                       unsigned int frame,             /* Boolean */
                       unsigned int semiColonNeeded,   /* Boolean */
                       unsigned int validationCode,    /* Boolean */
                       unsigned int lookbackSignature, /* Boolean */
                       unsigned int managedCPPCode
                      );

static void printExcelGlueCode( FILE *out, const TA_FuncInfo *funcInfo );
static void printCallFrame  ( FILE *out, const TA_FuncInfo *funcInfo );
static void printFrameHeader( FILE *out, const TA_FuncInfo *funcInfo );

static void printExternReferenceForEachFunction( const TA_FuncInfo *info,
                                                 void *opaqueData );

static void printFunctionAddress( const TA_FuncInfo *info,
                                  void *opaqueData );

static void printPerGroupList( const char *groupName,
                               unsigned int index,
                               unsigned int isFirst,
                               unsigned int isLast
                             );

static void printGroupListAddress(  const char *groupName,
                                    unsigned int index,
                                    unsigned int isFirst,
                                    unsigned int isLast
                                 );
static void printGroupSize(  const char *groupName,
                             unsigned int index,
                             unsigned int isFirst,
                             unsigned int isLast
                           );
static void printGroupSizeAddition(  const char *groupName,
                                     unsigned int index,
                                     unsigned int isFirst,
                                     unsigned int isLast
                                  );

static int addUnstablePeriodEnum( FILE *out );

static int createTemplate( FileHandle *in, FileHandle *out );
static void writeFuncFile( const TA_FuncInfo *funcInfo );
static void doFuncFile( const TA_FuncInfo *funcInfo );
static void printOptInputValidation( FILE *out,
                                     const char *name,
                                     unsigned int paramNb,
                                     const TA_OptInputParameterInfo *optInputParamInfo );
static int skipToGenCode( const char *dstName, FILE *out, FILE *templateFile );
static void printDefines( FILE *out, const TA_FuncInfo *funcInfo );

static void printFuncHeaderDoc( FILE *out,
                                const TA_FuncInfo *funcInfo,
                                const char *prefix );


static void addFuncEnumeration( FILE *out );

char gToOpen[1024];
char gTempBuf[2048];
char gTempBuf2[2048];
char gTempBuf3[2048];
char gTempDoubleToStr[50];

/* Because Microsoft and Borland does not display
 * the value of a double in the same way (%e), this
 * function attempts to eliminate difference. This
 * is done to avoid annoying difference with CVS. 
 */
const char *doubleToStr( double value );

const char *gCurrentGroupName;

static int genCode(int argc, char* argv[]);

extern const TA_OptInputParameterInfo TA_DEF_UI_MA_Method;

int main(int argc, char* argv[])
{
   int retValue;
   TA_InitializeParam param;
   TA_RetCode retCode;

   if( argc > 1 )
   {
         /* There is no parameter needed for this tool. */
         printf( "\n" );
         printf( "gen_code V%s - Updates many TA-LIB source files\n", TA_GetVersionString() );
         printf( "\n" );
         printf( "Usage: gen_code\n");
         printf( "\n" );         
         printf( "  No parameter needed.\n" );
         printf( "\n" );         
         printf( "  This utility is useful only for developers modifying\n" );
         printf( "  the source code of the TA-Lib.\n" );
         printf( "\n" );         
         printf( "  A lots of code is generated from the information\n" );
         printf( "  provided by the interface definitions found\n" );
         printf( "  in ta-lib/c/src/ta_abstract/tables.\n" );
         printf( "\n" );
         printf( "  From these tables, the following files are\n" );
         printf( "  updated:\n" );
         printf( "     1) ta-lib/c/include/ta_func.h\n" );
         printf( "     2) ta-lib/c/include/ta_defs.h\n" );
         printf( "     3) ta-lib/c/include/func_list.txt\n" );
         printf( "     4) ta-lib/c/src/ta_common/ta_retcode.*\n" );
         printf( "     5) ta-lib/c/src/ta_abstract/ta_group_idx.c\n");     
         printf( "     6) ta-lib/c/src/ta_abstract/frames/*.*\n");
         printf( "     7) ta-lib/c/src/ta_abstract/excel_glue.c\n" );
         printf( "\n" );
         printf( "  Also, it regenerates the function header, parameters and\n" );
         printf( "  validation code of all TA Func in c/src/ta_func.\n" );
         printf( "\n" );
         printf( "** Must be directly run from the 'bin' directory.\n" );
         exit(-1);
   }

   printf( "gen_code V%s\n", TA_GetVersionString() );

   memset( &param, 0, sizeof( TA_InitializeParam ) );
   param.logOutput = stdout;
   retCode = TA_Initialize( &param );
   if( retCode != TA_SUCCESS )
   {
      printf( "\nCannot initialize the library\n");
      return -1;
   }

   printf( "Now updating source code...\n" );

   retValue = genCode( argc, argv );
   TA_Shutdown();

   return retValue;
}


/* The following I/O function allows to manipulate
 * more easily files.
 *
 * When opening the file, the caller can specifiy a
 * path relative to the position of the binary.
 * That is: ta-lib\c\bin
 *
 * 'templateFile' allows to create a new file using
 * a template. This template must contain one
 * line starting with '%%%GENCODE%%%'.
 * All character before this string are added to the output
 * file on fileOpen, and all character after this string are
 * added to the output file on fileClose. Obviously, all
 * character added to the file between fileOpen/fileClose
 * will replace the "%%%GENCODE%%%" line.
 *
 * 'templateFile' is ignored when FILE_READ is specified.
 *
 * On failure, simply exit the software.
 */
static void init_gToOpen( const char *filePath )
{
   int sepChar;
   char *ptr;

   sepChar = TA_SeparatorASCII();

#if 0
   unsigned int strLen;
   char oneCharBuf[2];

   oneCharBuf[1] = '\0';

   strcpy( gToOpen, "..\\" );
   strLen = strlen( gToOpen );
   if( strLen > 0 )
   {
      if( (gToOpen[strLen-1] != '\\') &&
          (gToOpen[strLen-1] != '/') )
      {
         oneCharBuf[0] = (char)sepChar;
         strcat( gToOpen, oneCharBuf );
      }
   }
#endif
   strcpy( gToOpen, filePath );

   /* Replace all directory separator with the
    * one applicable for this OS.
    */
   ptr = gToOpen;
   while( *ptr != '\0' )
   {
      if( (*ptr == '\\') || (*ptr == '/') )
         *ptr = (char)sepChar;
      ptr++;
   }
}


static  FileHandle *fileOpen( const char *fileToOpen,
                              const char *templateFile,
                              const int readOnly )
{
   FileHandle *retValue;

   if( (fileToOpen == NULL) ||
       (readOnly && (templateFile != NULL)) )
   {
      printf( "Internal error line %d", __LINE__ );
      return (FileHandle *)NULL;
   }

   retValue = malloc( sizeof( FileHandle ) );

   retValue->file = NULL;
   retValue->templateFile = NULL;

   init_gToOpen( fileToOpen );

   /* Ok.. let's try to open the file now. */
   retValue->file = fopen( gToOpen, readOnly ? "r":"w" );
   if( retValue->file == NULL )
   {
      return (FileHandle *)NULL;
   }

   /* Handle the template. */
   if( templateFile )
   {
      init_gToOpen( templateFile );
      retValue->templateFile = fopen( gToOpen, "r" );
      if( retValue->templateFile == NULL )
      {
         printf( "\nCannot open template [%s]\n", gToOpen );
         return (FileHandle *)NULL;
      }

      /* Copy the header part of the template. */
      if( skipToGenCode( fileToOpen, retValue->file, retValue->templateFile ) != 0 )
      {
         free( retValue );
         retValue = NULL;
      }
   }

   return retValue;
}

static void fileClose( FileHandle *handle )
{
   /* Write remaining template info. */
   if( handle->templateFile )
   {
      while( fgets( gTempBuf, 2048, handle->templateFile ) != NULL )
      {
         if( fputs( gTempBuf, handle->file ) == EOF )
         {
            printf( "Cannot write to output file! Disk Full? " );
            break;
         }
      }

      /* Make sure the last line of th eoutput 
       * finish with a carriage return. This may
       * avoid warning from some compilers.
       */
      if( gTempBuf[0] != '\n' )
         fprintf( handle->file, "\n" );

      fclose( handle->templateFile );
   }

   fclose( handle->file );
   free( handle );
}

static int genCode(int argc, char* argv[])
{
   TA_RetCode retCode;
   unsigned int nbGroup;

   (void)argc; /* Get ride of compiler warning */
   (void)argv; /* Get ride of compiler warning */

   /* Create ta_retcode.c */
   if( gen_retcode() != 0 )
   {
      printf( "\nCannot generate src/ta_common/ta_retcode.c\n" );
      return -1;
   }

   /* Create "ta_func.h" */
   gOutFunc_H = fileOpen( "..\\include\\ta_func.h",
                          "..\\src\\ta_abstract\\templates\\ta_func.h.template",
                          FILE_WRITE );

   if( gOutFunc_H == NULL )
   {
      printf( "\nCannot access [%s]\n", gToOpen );
      return -1;
   }

   /* Create the "func_list.txt" */
   gOutFuncList_TXT = fileOpen( "..\\include\\func_list.txt",
                                NULL,
                                FILE_WRITE );

   if( gOutFuncList_TXT == NULL )
   {
      printf( "\nCannot access [%s]\n", gToOpen );
      return -1;
   }


   /* Create the "ta_frame.h" */
   gOutFrame_H = fileOpen( "..\\src\\ta_abstract\\frames\\ta_frame.h",
                           "..\\src\\ta_abstract\\templates\\ta_frame.h.template",
                           FILE_WRITE );

   if( gOutFrame_H == NULL )
   {
      printf( "\nCannot access [%s]\n", gToOpen );
      return -1;
   }

   /* Create the "ta_frame.c" */
   gOutFrame_C = fileOpen( "..\\src\\ta_abstract\\frames\\ta_frame.c",
                           "..\\src\\ta_abstract\\templates\\ta_frame.c.template",
                           FILE_WRITE );

   if( gOutFrame_C == NULL )
   {
      printf( "\nCannot access [%s]\n", gToOpen );
      return -1;
   }

   /* Create "excel_glue.c" */
   gOutExcelGlue_C = fileOpen( "..\\src\\ta_abstract\\excel_glue.c",
                           "..\\src\\ta_abstract\\templates\\excel_glue.c.template",
                           FILE_WRITE );

   if( gOutExcelGlue_C == NULL )
   {
      printf( "\nCannot access [%s]\n", gToOpen );
      return -1;
   }

   /* Process each function. */
   retCode = TA_ForEachFunc( doForEachFunction, NULL );

   if( retCode != TA_SUCCESS )
   {
      printf( "Failed [%d]\n", retCode );
      return -1;
   }
         
   fileClose( gOutFuncList_TXT );
   fileClose( gOutFunc_H );
   fileClose( gOutFrame_H );
   fileClose( gOutFrame_C );
   fileClose( gOutExcelGlue_C );

   if( retCode != TA_SUCCESS )
   {
      printf( "Failed [%d]\n", retCode );
      return -1;
   }


   /* Create the "ta_group_idx.c" file. */
   gOutGroupIdx_C = fileOpen( "..\\src\\ta_abstract\\ta_group_idx.c",
                              "..\\src\\ta_abstract\\templates\\ta_group_idx.c.template",
                              FILE_WRITE );

   if( gOutGroupIdx_C == NULL )
   {
      printf( "\nCannot access [%s]\n", gToOpen );
      return -1;
   }

   retCode = TA_ForEachFunc( printExternReferenceForEachFunction, NULL );
   if( retCode != TA_SUCCESS )
   {
      fileClose( gOutGroupIdx_C );
      return -1;
   }

   nbGroup = forEachGroup( printPerGroupList, NULL );

   fprintf( gOutGroupIdx_C->file, "const TA_FuncDef **TA_PerGroupFuncDef[%d] = {\n", nbGroup );
   forEachGroup( printGroupListAddress, NULL );
   fprintf( gOutGroupIdx_C->file, "};\n\n" );

   fprintf( gOutGroupIdx_C->file, "const unsigned int TA_PerGroupSize[%d] = {\n", nbGroup );
   forEachGroup( printGroupSize, NULL );
   fprintf( gOutGroupIdx_C->file, "};\n\n" );

   fprintf( gOutGroupIdx_C->file, "const unsigned int TA_TotalNbFunction =\n" );
   forEachGroup( printGroupSizeAddition, NULL );

   fileClose( gOutGroupIdx_C );

   /* Update "ta_defs.h" */
   doDefsFile();

   printf( "\n** Update completed with success **\n");

   return 0;
}

static unsigned int forEachGroup( TA_ForEachGroup forEachGroupFunc,
                                  void *opaqueData )
{
   TA_RetCode retCode;
   TA_StringTable *table;
   unsigned int i;   

   (void)opaqueData; /* Get ride of compiler warning */

   retCode = TA_GroupTableAlloc( &table );
   if( retCode != TA_SUCCESS )
      return 0;

   for( i=0; i < table->size; i++ )
   {
      forEachGroupFunc( table->string[i],
                        i,
                        i==0? 1:0,
                        i==(table->size-1)? 1:0 );
   }
   
   retCode = TA_GroupTableFree ( table );
   if( retCode != TA_SUCCESS )
      return 0;

   return i;
}

static void doForEachFunction( const TA_FuncInfo *funcInfo,
                               void *opaqueData )
{
   static const char *prevGroup = NULL;

   (void)opaqueData; /* Get ride of compiler warning */

   /* Add this function to the "func_list.txt" */
   fprintf( gOutFuncList_TXT->file, "%-20s%s\n", funcInfo->name, funcInfo->hint );
  
   fprintf( gOutFunc_H->file, "\n" );

   if( (prevGroup == NULL) || (prevGroup != funcInfo->group) )
   {
      // printf( "Processing Group [%s]\n", funcInfo->group );
      fprintf( gOutFunc_H->file, "\n/******************************************\n" );
      fprintf( gOutFunc_H->file, " * Group: [%s]\n", funcInfo->group );
      fprintf( gOutFunc_H->file, " ******************************************/\n\n" );

      prevGroup = funcInfo->group;
   }

   // printf( "Processing Func  [TA_%s]\n", funcInfo->name );

   fprintf( gOutFunc_H->file, "/*\n" );
   printFuncHeaderDoc( gOutFunc_H->file, funcInfo, " * " );
   fprintf( gOutFunc_H->file, " */\n" );

   /* Generate the defines corresponding to this function. */
   printDefines( gOutFunc_H->file, funcInfo );

   /* Generate the function prototype. */
   printFunc( gOutFunc_H->file, NULL, funcInfo, 1, 0, 1, 0, 0, 0 );
   fprintf( gOutFunc_H->file, "\n" );

   /* Generate the corresponding lookback function prototype. */
   printFunc( gOutFunc_H->file, NULL, funcInfo, 1, 0, 1, 0, 1, 0 );

   /* Generate the excel glue code */
   printExcelGlueCode( gOutExcelGlue_C->file, funcInfo );

   /* Create the frame definition (ta_frame.c) and declaration (ta_frame.h) */
   printFrameHeader( gOutFrame_H->file, funcInfo );
   fprintf( gOutFrame_H->file, ";\n\n" );
   printCallFrame( gOutFrame_C->file, funcInfo );

   doFuncFile( funcInfo );
}

static void doForEachUnstableFunction( const TA_FuncInfo *funcInfo,
                                       void *opaqueData )
{
   unsigned int *i;

   i = (unsigned int *)opaqueData;

   if( funcInfo->flags & TA_FUNC_FLG_UNST_PER )
   {
      fprintf( gOutDefs_H->file, "    /* %03d */  TA_FUNC_UNST_%s,\n", *i, funcInfo->name );
      (*i)++;
   }
}

static void printIndent( FILE *out, unsigned int indent )
{
   unsigned int i;

   for( i=0; i < indent; i++ )
      fprintf( out, " " );
}

static void printDefines( FILE *out, const TA_FuncInfo *funcInfo )
{
   TA_RetCode retCode;
   const TA_OptInputParameterInfo *optInputParamInfo;
   unsigned int i, j;
   unsigned int paramNb;
   const char *paramName;
   const char *defaultParamName;
   TA_IntegerList *intList;
   TA_RealList    *realList;

   /* Go through the optional parameter and print
    * the corresponding define for the TA_OptInput_IntegerList
    * and TA_OptInput_RealList having a string.
    */
   paramNb = 0;
   for( i=0; i < funcInfo->nbOptInput; i++ )
   {
      retCode = TA_SetOptInputParameterInfoPtr( funcInfo->handle,
                                                i, &optInputParamInfo );

      if( retCode != TA_SUCCESS )
      {
         printf( "[%s] invalid 'optional input' information\n", funcInfo->name );
         return;
      }

      paramName = optInputParamInfo->paramName;

      /* TA_MA: Value for parameter */

      switch( optInputParamInfo->type )
      {
      case TA_OptInput_RealList:
         defaultParamName = "optInReal";
         break;
      case TA_OptInput_IntegerList:
         defaultParamName = "optInInteger";
         break;
      default:
         paramNb++;
         continue; /* Skip other type of parameter */
      }

      if( !paramName )
         paramName = defaultParamName;

      /* Output a comment to guide the user. */
         switch( optInputParamInfo->type )
         {
         case TA_OptInput_IntegerList:
            intList = (TA_IntegerList *)optInputParamInfo->dataSet;
            if( intList != (TA_IntegerList *)TA_DEF_UI_MA_Method.dataSet )
            {
               fprintf( out, "\n/* TA_%s: Optional Parameter %s_%d */\n",
                        funcInfo->name, paramName, paramNb );
               for( j=0; j < intList->nbElement; j++ )
               {
                  strcpy( gTempBuf, intList->data[j].string );
                  strconvch( gTempBuf, ' ', '_' );
                  trim( gTempBuf );
                  strupc( gTempBuf );
                  fprintf( out, "#define TA_%s_%s %d\n",
                           funcInfo->name,
                           gTempBuf,
                           intList->data[j].value );

               }
               fprintf( out, "\n" );
            }
            break;
         case TA_OptInput_RealList:
            fprintf( out, "\n/* TA_%s: Optional Parameter %s_%d */\n",
                     funcInfo->name, paramName, paramNb );

            realList = (TA_RealList *)optInputParamInfo->dataSet;
            for( j=0; j < realList->nbElement; j++ )
            {
               strcpy( gTempBuf, realList->data[j].string );
               strconvch( gTempBuf, ' ', '_' );
               trim( gTempBuf );
               strupc( gTempBuf );
               fprintf( out, "#define TA_%s_%s %s\n",
                        funcInfo->name,
                        gTempBuf,
                        doubleToStr(realList->data[j].value) );

            }
            fprintf( out, "\n" );
            break;
         default:
            /* Do nothing */
            break;
         }

      paramNb++;
   }
}

static void printFunc( FILE *out,
                       const char *prefix, /* Can be NULL */
                       const TA_FuncInfo *funcInfo,
                       unsigned int prototype, /* Boolean */
                       unsigned int frame,     /* Boolean */
                       unsigned int semiColonNeeded, /* Boolean */
                       unsigned int validationCode, /* Boolean */
                       unsigned int lookbackSignature, /* Boolean */
                       unsigned int managedCPPCode
                      )
{
   TA_RetCode retCode;
   unsigned int i, j, k, indent, lastParam;
   unsigned int paramNb;
   const char *paramName;
   const char *defaultParamName;
   const TA_InputParameterInfo *inputParamInfo;
   const TA_OptInputParameterInfo *optInputParamInfo;
   const TA_OutputParameterInfo *outputParamInfo;
   const char *typeString;
   const char *inputDoubleArrayType;
   const char *inputIntArrayType;
   const char *outputIntParam;
   const char *arrayBracket;

   if( managedCPPCode )
   {
      inputDoubleArrayType  = "double";
      inputIntArrayType     = "int";
      outputIntParam        = "[OutAttribute]Int32";
      arrayBracket          = " __gc []";
   }
   else
   {
      inputDoubleArrayType  = "const double";
      inputIntArrayType     = "const int";
      outputIntParam        = "int";
      arrayBracket          = "[]";
   }

   typeString = "";
   defaultParamName = "";

   if( prototype )
   {
      if( lookbackSignature )
      {         
         sprintf( gTempBuf, "%sint %s%s_Lookback( ",
                  prefix? prefix:"",
                  managedCPPCode? "Core::":"TA_",
                  funcInfo->name );
         fprintf( out, gTempBuf );
         indent = strlen(gTempBuf) - 2;
      }
      else
      {
         sprintf( gTempBuf, "%s%s%s( int    startIdx,\n",
                  prefix? prefix:"",
                  managedCPPCode? "enum TA_RetCode Core::":"TA_RetCode TA_",
                  funcInfo->name );
         fprintf( out, gTempBuf );
         indent = strlen(gTempBuf) - 17;
         printIndent( out, indent );
         fprintf( out, "int    endIdx,\n" );
      }
   }
   else if( frame )
   {
      fprintf( out, "%sTA_%s(\n",
               prefix == NULL? "" : prefix, funcInfo->name );
      indent = 4 + strlen(funcInfo->name);
   }
   else if( validationCode )
   {
      indent = 3;
   }
   else
   {
      printf( "printFunc has nothing to do?\n" );
      return;
   }

   if( prefix )
      indent += strlen(prefix);
   if( frame )
      indent -= 5;

   if( frame )
   {
      printIndent( out, indent );
      fprintf( out, "startIdx,\n" );
      printIndent( out, indent );
      fprintf( out, "endIdx,\n" );
   }

   /* Go through all the input. */
   if( !lookbackSignature )
   {
      paramNb = 0;
      for( i=0; i < funcInfo->nbInput; i++ )
      {
         retCode = TA_SetInputParameterInfoPtr( funcInfo->handle,
                                                i, &inputParamInfo );

         if( retCode != TA_SUCCESS )
         {
            printf( "[%s] invalid 'input' information (%d,%d)\n", funcInfo->name, i, paramNb );
            return;
         }

         paramName = inputParamInfo->paramName;

         switch( inputParamInfo->type )
         {
         case TA_Input_Price:
            /* Find how many component are requested. */
            j = 0;
            if( inputParamInfo->flags & TA_IN_PRICE_TIMESTAMP )
               j++;
            if( inputParamInfo->flags & TA_IN_PRICE_OPEN )
               j++;
            if( inputParamInfo->flags & TA_IN_PRICE_HIGH )
               j++;
            if( inputParamInfo->flags & TA_IN_PRICE_LOW )
               j++;
            if( inputParamInfo->flags & TA_IN_PRICE_CLOSE )
               j++;
            if( inputParamInfo->flags & TA_IN_PRICE_VOLUME )
               j++;
            if( inputParamInfo->flags & TA_IN_PRICE_OPENINTEREST )
               j++;

            if( j == 0 )
            {
               printf( "[%s] invalid 'price input' information (%d,%d)\n", funcInfo->name, i, paramNb );
               return;
            }

            if( validationCode )
            {
               printIndent( out, indent );
               fprintf( out, "/* Verify required price component. */\n" );
               printIndent( out, indent );
               fprintf( out, "if(" );
               k = 0;
               if( inputParamInfo->flags & TA_IN_PRICE_TIMESTAMP )
               {
                  k++;
                  fprintf( out, "!inTimestamp_%d%s", paramNb, k != j? "||":")");
               }

               if( inputParamInfo->flags & TA_IN_PRICE_OPEN )
               {
                  k++;
                  fprintf( out, "!inOpen_%d%s", paramNb, k != j? "||":")");
               }
               
               if( inputParamInfo->flags & TA_IN_PRICE_HIGH )
               {
                  k++;
                  fprintf( out, "!inHigh_%d%s", paramNb, k != j? "||":")");
               }

               if( inputParamInfo->flags & TA_IN_PRICE_LOW )
               {
                  k++;
                  fprintf( out, "!inLow_%d%s", paramNb, k != j? "||":")");
               }

               if( inputParamInfo->flags & TA_IN_PRICE_CLOSE )
               {
                  k++;
                  fprintf( out, "!inClose_%d%s", paramNb, k != j? "||":")");
               }

               if( inputParamInfo->flags & TA_IN_PRICE_VOLUME )
               {
                  k++;
                  fprintf( out, "!inVolume_%d%s", paramNb, k != j? "||":")");
               }

               if( inputParamInfo->flags & TA_IN_PRICE_OPENINTEREST )
               {
                  k++;
                  fprintf( out, "!inOpenInterest_%d%s", paramNb, k != j? "||":")");
               }

               fprintf( out, "\n" );
               printIndent( out, indent );
               fprintf( out, "   return TA_BAD_PARAM;\n\n" );
            }
            else
            {
               if( inputParamInfo->flags & TA_IN_PRICE_TIMESTAMP )
               {
                  printIndent( out, indent );
                  if( frame )
                     fprintf( out, "params->in[%d].data.inPrice.timestamp, /*", paramNb );
                  fprintf( out, "%-*s %s_%d%s",
                           prototype? 12 : 0,
                           prototype? "const TA_Timestamp" : "",                           
                           "inTimestamp",
                           paramNb,
                           prototype? arrayBracket : "" );
                  fprintf( out, "%s\n", frame? " */":"," );
               }

               if( inputParamInfo->flags & TA_IN_PRICE_OPEN )
               {
                  printIndent( out, indent );
                  if( frame )
                     fprintf( out, "params->in[%d].data.inPrice.open, /*", paramNb );
                  fprintf( out, "%-*s %s_%d%s",
                           prototype? 12 : 0,
                           prototype? inputDoubleArrayType : "",
                           "inOpen",
                           paramNb,
                           prototype? arrayBracket : "" );
                  fprintf( out, "%s\n", frame? " */":"," );
               }

               if( inputParamInfo->flags & TA_IN_PRICE_HIGH )
               {
                  printIndent( out, indent );
                  if( frame )
                     fprintf( out, "params->in[%d].data.inPrice.high, /*", paramNb );
                  fprintf( out, "%-*s %s_%d%s",
                           prototype? 12 : 0,
                           prototype? inputDoubleArrayType : "",                           
                           "inHigh",
                           paramNb,
                           prototype? arrayBracket : "" );
                  fprintf( out, "%s\n", frame? " */":"," );
               }

               if( inputParamInfo->flags & TA_IN_PRICE_LOW )
               {
                  printIndent( out, indent );
                  if( frame )
                     fprintf( out, "params->in[%d].data.inPrice.low, /*", paramNb );
                  fprintf( out, "%-*s %s_%d%s",
                           prototype? 12 : 0,
                           prototype? inputDoubleArrayType : "",
                           "inLow",
                           paramNb,
                           prototype? arrayBracket : "" );
                  fprintf( out, "%s\n", frame? " */":"," );
               }

               if( inputParamInfo->flags & TA_IN_PRICE_CLOSE )
               {
                  printIndent( out, indent );
                  if( frame )
                     fprintf( out, "params->in[%d].data.inPrice.close, /*", paramNb );
                  fprintf( out, "%-*s %s_%d%s",
                           prototype? 12 : 0,
                           prototype? inputDoubleArrayType : "",                           
                           "inClose",
                           paramNb,
                           prototype? arrayBracket : "" );
                  fprintf( out, "%s\n", frame? " */":"," );
               }

               if( inputParamInfo->flags & TA_IN_PRICE_VOLUME )
               {
                  printIndent( out, indent );
                  if( frame )
                     fprintf( out, "params->in[%d].data.inPrice.volume, /*", paramNb );
                  fprintf( out, "%-*s %s_%d%s",
                           prototype? 12 : 0,
                           prototype? inputIntArrayType : "",
                           "inVolume",
                           paramNb,
                           prototype? arrayBracket : "" );
                  fprintf( out, "%s\n", frame? " */":"," );
               }

               if( inputParamInfo->flags & TA_IN_PRICE_OPENINTEREST )
               {
                  printIndent( out, indent );
                  if( frame )
                     fprintf( out, "params->in[%d].data.inPrice.openInterest, /*", paramNb );
                  fprintf( out, "%-*s %s_%d%s",
                           prototype? 12 : 0,
                           prototype? inputIntArrayType : "",
                           "inOpenInterest",
                           paramNb,
                           prototype? arrayBracket : "" );
                  fprintf( out, "%s\n", frame? " */":"," );
               }
            }
            break;
         case TA_Input_Real:
            typeString = inputDoubleArrayType;
            defaultParamName = "inReal";
            break;
         case TA_Input_Integer:
            typeString = inputIntArrayType;
            defaultParamName = "inInteger";
            break;
         /*case TA_Input_Timestamp:
            typeString = "const TA_Timestamp";
            defaultParamName = "inTimestamp";
            break;*/
         default:
            if( !paramName )
               paramName = "inParam";
            printf( "[%s,%s_%d] invalid 'input' type(%d)\n",
                    funcInfo->name, paramName, paramNb,
                    inputParamInfo->type );
            return;
         }

         if( inputParamInfo->type != TA_Input_Price )
         {
            printIndent( out, indent );
            if( validationCode )
               fprintf( out, "if( !%s_%d ) return TA_BAD_PARAM;\n", inputParamInfo->paramName, paramNb );
            else
            {
               if( frame )
                  fprintf( out, "params->in[%d].data.%s, /*", paramNb, defaultParamName );
               fprintf( out, "%-*s %s_%d%s",
                        prototype? 12 : 0,
                        prototype? typeString : "",                        
                        inputParamInfo->paramName,
                        paramNb, prototype? arrayBracket : "" );
               fprintf( out, "%s\n", frame? " */":"," );
            }
         }
         paramNb++;
      }
   }

   /* Go through all the optional input */
   paramNb = 0;
   lastParam = 0;
   for( i=0; i < funcInfo->nbOptInput; i++ )
   {
      if( (i == (funcInfo->nbOptInput-1)) && lookbackSignature )
         lastParam = 1;

      retCode = TA_SetOptInputParameterInfoPtr( funcInfo->handle,
                                                i, &optInputParamInfo );

      if( retCode != TA_SUCCESS )
      {
         printf( "[%s] invalid 'optional input' information\n", funcInfo->name );
         return;
      }

      paramName = optInputParamInfo->paramName;

      switch( optInputParamInfo->type )
      {
      case TA_OptInput_RealRange:
      case TA_OptInput_RealList:
         typeString = "double";
         defaultParamName = "optInReal";
         break;
      case TA_OptInput_IntegerRange:
         typeString = "int";
         defaultParamName = "optInInteger";
         break;
      case TA_OptInput_IntegerList:
         if( (optInputParamInfo->dataSet == TA_DEF_UI_MA_Method.dataSet) && !frame )
         {
            typeString = "TA_MAType";
            defaultParamName = "optInMAType";
         }
         else
         {
            typeString = "int";
            defaultParamName = "optInInteger";
         }
         break;
      default:
         if( !paramName )
            paramName = "optInParam";
         printf( "[%s,%s_%d] invalid 'optional input' type(%d)\n",
                 funcInfo->name, paramName, paramNb,
                 optInputParamInfo->type );
         return;
      }

      if( !paramName )
         paramName = defaultParamName;

      if( validationCode )
         printOptInputValidation( out, paramName, paramNb, optInputParamInfo );
      else
      {
         if( !(lookbackSignature && (i == 0 )) )
            printIndent( out, indent );

         if( frame )
            fprintf( out, "params->optIn[%d].data.%s, /*", paramNb, defaultParamName );
         fprintf( out, "%-*s %s_%d",
                  prototype? 13 : 0,
                  prototype? typeString : "",
                  paramName, paramNb );
         if( frame )
            fprintf( out, " */\n" );
         else            
         {
            switch( optInputParamInfo->type )
            {
            case TA_OptInput_RealRange:
               if( lookbackSignature && lastParam )
                  fprintf( out, " )%s ", semiColonNeeded? ";":"" );
               else
                  fprintf( out, "," );

               if( ((TA_RealRange *)(optInputParamInfo->dataSet))->min == TA_REAL_MIN )
                  fprintf( out, " /* From TA_REAL_MIN" );
               else
                  fprintf( out, " /* From %.*g",
                           ((TA_RealRange *)(optInputParamInfo->dataSet))->precision,
                           ((TA_RealRange *)(optInputParamInfo->dataSet))->min );

               if( ((TA_RealRange *)(optInputParamInfo->dataSet))->max == TA_REAL_MAX )
                  fprintf( out, " to TA_REAL_MAX */\n" );
               else
               {
                  fprintf( out, " to %.*g%s */\n", 
                          ((TA_RealRange *)(optInputParamInfo->dataSet))->precision,
                          ((TA_RealRange *)(optInputParamInfo->dataSet))->max,
                          optInputParamInfo->flags & TA_OPTIN_IS_PERCENT? " %":"" );
               }
               break;
            case TA_OptInput_IntegerRange:
               if( lookbackSignature && lastParam )
                  fprintf( out, " )%s ", semiColonNeeded? ";":"" );
               else
                  fprintf( out, "," );

               if( ((TA_IntegerRange *)(optInputParamInfo->dataSet))->min == TA_INTEGER_MIN )
                  fprintf( out, " /* From TA_INTEGER_MIN" );
               else
               {
                  fprintf( out, " /* From %d",
                           ((TA_IntegerRange *)(optInputParamInfo->dataSet))->min );
               }

               if( ((TA_IntegerRange *)(optInputParamInfo->dataSet))->max == TA_INTEGER_MAX )
                  fprintf( out, " to TA_INTEGER_MAX */\n" );
               else
               {
                  fprintf( out, " to %d */\n", 
                          ((TA_IntegerRange *)(optInputParamInfo->dataSet))->max );
               }
               break;
            default:
               if( lookbackSignature && lastParam )
                  fprintf( out, " )%s ", semiColonNeeded? ";":"" );
               else
                  fprintf( out, ",\n" );
            }
         }
      }

      paramNb++;
   }

   if( lookbackSignature && (funcInfo->nbOptInput == 0) )
   {
      fprintf( out, "void )%s\n", semiColonNeeded? ";":"" );
   }

   /* Go through all the output */
   if( lookbackSignature )
   {
      fprintf( out, "\n" );
   }
   else
   {
      paramNb = 0;
      lastParam = 0;

      if( !validationCode )
      {
            printIndent( out, indent );
            if( frame )
               fprintf( out, "outBegIdx, " );
            else
               fprintf( out, "%-*s %soutBegIdx",
                        prototype? 12 : 0,
                        prototype? outputIntParam : "",
                        prototype? "*" : "" );

            fprintf( out, "%s\n", frame? "":"," );

            printIndent( out, indent );
            if( frame )
               fprintf( out, "outNbElement, " );
            else
               fprintf( out, "%-*s %soutNbElement",
                        prototype? 12 : 0,
                        prototype? outputIntParam : "",
                        prototype? "*" : "" );
            fprintf( out, "%s\n", frame? "":"," );
      }

      for( i=0; i < funcInfo->nbOutput; i++ )
      {
         if( i == (funcInfo->nbOutput-1) )
            lastParam = 1;

         retCode = TA_SetOutputParameterInfoPtr( funcInfo->handle,
                                                 i, &outputParamInfo );

         if( retCode != TA_SUCCESS )
         {
            printf( "[%s] invalid 'output' information\n", funcInfo->name );
            return;
         }

         paramName = outputParamInfo->paramName;

         switch( outputParamInfo->type )
         {
         case TA_Output_Real:
            typeString = "double";
            defaultParamName = "outReal";
            break;
         case TA_Output_Integer:
            typeString = "int";
            defaultParamName = "outInteger";
            break;
         default:
            if( !paramName )
               paramName = "outParam";
            printf( "[%s,%s_%d] invalid 'output' type(%d)\n",
                    funcInfo->name, paramName, paramNb,
                    outputParamInfo->type );
            return;
         }

         if( !paramName )
            paramName = defaultParamName;

         if( validationCode )
         {
            fprintf( out, "   if( %s_%d == NULL )\n", paramName, paramNb );
            fprintf( out, "      return TA_BAD_PARAM;\n\n" );
         }
         else
         {
            printIndent( out, indent );
            if( frame )
               fprintf( out, "params->out[%d].data.%s%s /*",
                        paramNb, defaultParamName,
                        lastParam? "":"," );

            fprintf( out, "%-*s  %s_%d%s",
                     prototype? 12 : 0,
                     prototype? typeString : "",                     
                     paramName, paramNb,
                     prototype? arrayBracket : "" );

            if( !lastParam )
               fprintf( out, "%s\n", frame? " */":"," );
            else
            {
               fprintf( out, "%s )%s\n",
                        frame? " */":"",
                        semiColonNeeded? ";":"" );
            }
         }

         paramNb++;
      }
   }
}

static void printCallFrame( FILE *out, const TA_FuncInfo *funcInfo )
{
   printFrameHeader( out, funcInfo );
   fprintf( out, "\n{\n" );
   printFunc( out, "   return ", funcInfo, 0, 1, 1, 0, 0, 0 );
   fprintf( out, "}\n" );
}


static void printFrameHeader( FILE *out, const TA_FuncInfo *funcInfo )
{
   fprintf( out, "TA_RetCode TA_%s_FramePP( const TA_ParamHolderPriv *params,\n", funcInfo->name );
   fprintf( out, "                          int            startIdx,\n" );
   fprintf( out, "                          int            endIdx,\n" );
   fprintf( out, "                          int           *outBegIdx,\n" );
   fprintf( out, "                          int           *outNbElement )\n" );
}

static void printExternReferenceForEachFunction( const TA_FuncInfo *info,
                                                 void *opaqueData )
{
   (void)opaqueData; /* Get ride of compiler warning */

   fprintf( gOutGroupIdx_C->file, "extern const TA_FuncDef TA_DEF_%s;\n", info->name );
}

static void printPerGroupList( const char *groupName,
                               unsigned int index,
                               unsigned int isFirst,
                               unsigned int isLast
                             )
{
   (void)isLast; /* Get ride of compiler warning. */
   (void)isFirst; /* Get ride of compiler warning. */

   fprintf( gOutGroupIdx_C->file,
           "\nconst TA_FuncDef *TA_PerGroupFunc_%d[] = {\n", index );

   gCurrentGroupName = groupName;
   TA_ForEachFunc( printFunctionAddress, NULL );
   fprintf( gOutGroupIdx_C->file, "NULL };\n" );

   fprintf( gOutGroupIdx_C->file,
      "#define SIZE_GROUP_%d ((sizeof(TA_PerGroupFunc_%d)/sizeof(const TA_FuncDef *))-1)\n",
      index, index );
}

static void printFunctionAddress( const TA_FuncInfo *info,
                                  void *opaqueData )
{
   (void)opaqueData; /* Get ride of compiler warning. */

   if( strcmp( info->group, gCurrentGroupName ) == 0 )
      fprintf( gOutGroupIdx_C->file, "&TA_DEF_%s,\n", info->name );
}

static void printGroupListAddress( const char *groupName,
                                   unsigned int index,
                                   unsigned int isFirst,
                                   unsigned int isLast
                                  )
{
   (void)isFirst;   /* Get ride of compiler warning. */

   if( groupName == NULL )
      fprintf( gOutGroupIdx_C->file, "NULL%s", isLast? "" : "," );
   else fprintf( gOutGroupIdx_C->file, "&TA_PerGroupFunc_%d[0]%s\n",
                 index, isLast? "" : "," );
}

static void printGroupSize( const char *groupName,
                            unsigned int index,
                            unsigned int isFirst,
                            unsigned int isLast
                           )
{
   (void)isFirst;   /* Get ride of compiler warning. */
   (void)groupName; /* Get ride of compiler warning. */

   fprintf( gOutGroupIdx_C->file, "SIZE_GROUP_%d%s\n",
            index, isLast? "" : "," );
}

static void printGroupSizeAddition( const char *groupName,
                                    unsigned int index,
                                    unsigned int isFirst,
                                    unsigned int isLast
                                   )
{
   (void)isFirst;   /* Get ride of compiler warning. */
   (void)groupName; /* Get ride of compiler warning. */

   fprintf( gOutGroupIdx_C->file, "SIZE_GROUP_%d%s",
            index, isLast? ";" : "+\n" );
}

static void doFuncFile( const TA_FuncInfo *funcInfo )
{

   FileHandle *tempFile;
   unsigned int useTempFile;

   /* Check if the file already exist. */
   sprintf( gTempBuf, "..\\src\\ta_func\\ta_%s.c", funcInfo->name );

   gOutFunc_C = fileOpen( gTempBuf, NULL, FILE_READ );
   if( gOutFunc_C == NULL )
      useTempFile = 0;
   else
   {
      useTempFile = 1;
      /* Create a temporary template using it. */
      sprintf( gTempBuf, "..\\temp\\ta_%s.tmp", funcInfo->name );

      tempFile = fileOpen( gTempBuf, NULL, FILE_WRITE );
      if( tempFile == NULL )
      {
         printf( "Cannot create temporary file!\n" );
         return;
      }

      createTemplate( gOutFunc_C, tempFile );

      fileClose( tempFile );
      fileClose( gOutFunc_C );
   }

   /* Open the file using the template. */
   if( useTempFile )
      sprintf( gTempBuf2, "..\\temp\\ta_%s.tmp", funcInfo->name );
   else
      sprintf( gTempBuf2, "..\\src\\ta_abstract\\templates\\ta_x.c.template" );

   sprintf( gTempBuf, "..\\src\\ta_func\\ta_%s.c", funcInfo->name );

   gOutFunc_C = fileOpen( gTempBuf, gTempBuf2, FILE_WRITE );

   if( gOutFunc_C == NULL )
   {
      printf( "Cannot create [%s]\n", gTempBuf );
      return;
   }

   writeFuncFile( funcInfo );

   fileClose( gOutFunc_C );
}

static void doDefsFile( void )
{

   FileHandle *tempFile;
   FILE *out;
   
   #define FILE_TA_DEFS_H    "..\\include\\ta_defs.h"
   #define FILE_TA_DEFS_TMP  "..\\temp\\ta_defs.tmp"

   /* Check if the file already exist. If not, this is an error. */
   gOutDefs_H = fileOpen( FILE_TA_DEFS_H, NULL, FILE_READ );
   if( gOutDefs_H == NULL )
   {
      printf( "ta_defs.h must exist for being updated!\n" );
      exit(-1);
   }

   /* Create the template. The template is just the original file content
    * with the GENCODE SECTION emptied (so they can be re-generated)
    */
   tempFile = fileOpen( FILE_TA_DEFS_TMP, NULL, FILE_WRITE );
   if( tempFile == NULL )
   {
      printf( "Cannot create temporary file!\n" );
      exit(-1);
   }

   createTemplate( gOutDefs_H, tempFile );

   fileClose( tempFile );
   fileClose( gOutDefs_H );

   /* Re-open the file using the template. */
   gOutDefs_H = fileOpen( FILE_TA_DEFS_H, FILE_TA_DEFS_TMP, FILE_WRITE );
                                                    
   if( gOutDefs_H == NULL )
   {
      printf( "Cannot create ta_defs.h\n" );
      exit(-1);
   }

   /* Generate the GENCODE SECTION */
   out = gOutDefs_H->file;
   
   addFuncEnumeration( out );
   addUnstablePeriodEnum( out );

   fileClose( gOutDefs_H );
   #undef FILE_TA_DEFS_H
   #undef FILE_TA_DEFS_TMP
}

static int createTemplate( FileHandle *in, FileHandle *out )
{
   FILE *inFile;
   FILE *outFile;
   unsigned int skipSection;
   unsigned int sectionDone;

   inFile = in->file;
   outFile = out->file;

   skipSection = 0;
   sectionDone = 0;
   while( fgets( gTempBuf, 2048, inFile ) )
   {
      if( strncmp( gTempBuf, "/**** START GENCODE SECTION", 27 ) == 0 )
      {
         skipSection = 1;
         fputs( gTempBuf, outFile );
         fputs( "%%%GENCODE%%%\n", outFile );
      }

      else if( strncmp( gTempBuf, "/**** END GENCODE SECTION", 25 ) == 0 )
      {
         if( skipSection )
         {
            skipSection = 0;
            sectionDone++;
         }
      }

      if( !skipSection )
      {
         if( fputs( gTempBuf, outFile ) == EOF )
         {
            printf( "Cannot write tmp file\n" );
            return -1;
         }
      }
   }

   return 0;
}

static void writeFuncFile( const TA_FuncInfo *funcInfo )
{
   FILE *out;

   out = gOutFunc_C->file;
   fprintf( out, "/* All code within this section is automatically\n" );
   fprintf( out, " * generated by gen_code. Any modification will be lost\n" );
   fprintf( out, " * next time gen_code is run.\n" );
   fprintf( out, " */\n" );
   fprintf( out, "\n" );
   fprintf( out, "#if defined( _MANAGED )\n" );
   fprintf( out, "   #using <mscorlib.dll>\n" );
   fprintf( out, "   #include \"Core.h\"\n" );
   fprintf( out, "   namespace TA { namespace Lib {\n" );
   fprintf( out, "#else\n" );
   fprintf( out, "   #include \"ta_func.h\"\n" );
   fprintf( out, "#endif\n" );
   fprintf( out, "\n" );
   fprintf( out, "#ifndef TA_UTILITY_H\n" );
   fprintf( out, "   #include \"ta_utility.h\"\n" );
   fprintf( out, "#endif\n" );
   fprintf( out, "\n" );
   fprintf( out, "#if defined( _MANAGED )\n" );
   printFunc( out, NULL, funcInfo, 1, 0, 0, 0, 1, 1 );
   fprintf( out, "#else\n" );
   printFunc( out, NULL, funcInfo, 1, 0, 0, 0, 1, 0 );
   fprintf( out, "#endif\n" );
   skipToGenCode( funcInfo->name, gOutFunc_C->file, gOutFunc_C->templateFile );

   fprintf( out, "/*\n" );
   printFuncHeaderDoc( out, funcInfo, " * " );
   fprintf( out, " */\n" );
   fprintf( out, "\n" );

   fprintf( out, "\n" );
   fprintf( out, "#if defined( _MANAGED )\n" );
   printFunc( out, NULL, funcInfo, 1, 0, 0, 0, 0, 1 );
   fprintf( out, "#else\n" );
   printFunc( out, NULL, funcInfo, 1, 0, 0, 0, 0, 0 );
   fprintf( out, "#endif\n" );
   skipToGenCode( funcInfo->name, gOutFunc_C->file, gOutFunc_C->templateFile );

   fprintf( out, "\n" );
   fprintf( out, "#ifndef TA_FUNC_NO_RANGE_CHECK\n" );
   fprintf( out, "\n" );
   fprintf( out, "   /* Validate the requested output range. */\n" );
   fprintf( out, "   if( startIdx < 0 )\n" );
   fprintf( out, "      return TA_OUT_OF_RANGE_START_INDEX;\n" );
   fprintf( out, "   if( (endIdx < 0) || (endIdx < startIdx))\n" );
   fprintf( out, "      return TA_OUT_OF_RANGE_END_INDEX;\n" );
   fprintf( out, "\n" );
   /* Generate the code for checking the parameters.
    * Also generates the code for setting up the
    * default values.
    */
   fprintf( out, "   /* Validate the parameters. */\n" );
   printFunc( out, NULL, funcInfo, 0, 0, 0, 1, 0, 0 );

   fprintf( out, "#endif /* TA_FUNC_NO_RANGE_CHECK */\n" );
   fprintf( out, "\n" );

   /* Add the suffix at the end of the file. */
   skipToGenCode( funcInfo->name, gOutFunc_C->file, gOutFunc_C->templateFile );
   fprintf( out, "#if defined( _MANAGED )\n" );
   fprintf( out, "   }} // Close namespace TA.Lib\n" );
   fprintf( out, "#endif\n" );
}

static void printOptInputValidation( FILE *out,
                                     const char *name,
                                     unsigned int paramNb,
                                     const TA_OptInputParameterInfo *optInputParamInfo )
{
   int minInt, maxInt;
   double minReal, maxReal;
   unsigned int i;

   const TA_RealList     *realList;
   const TA_IntegerList  *integerList;
   const TA_RealRange *realRange;
   const TA_IntegerRange *integerRange;

   minInt  = maxInt  = (int)0;
   minReal = maxReal = (double)0.0;

   switch( optInputParamInfo->type )
   {
   case TA_OptInput_RealRange:
      realRange = (const TA_RealRange *)optInputParamInfo->dataSet;
      minReal = realRange->min;
      maxReal = realRange->max;
      break;
   case TA_OptInput_IntegerRange:
      integerRange = (const TA_IntegerRange *)optInputParamInfo->dataSet;
      minInt = integerRange->min;
      maxInt = integerRange->max;
      break;
   case TA_OptInput_RealList:
      /* Go through the list to find the min/max. */
      realList = (const TA_RealList *)optInputParamInfo->dataSet;
      minReal = realList->data[0].value;
      maxReal = realList->data[0].value;
      for( i=0; i < realList->nbElement; i++ )
      {
         minReal = min( realList->data[i].value, minReal );
         maxReal = max( realList->data[i].value, maxReal );
      }
      break;
   case TA_OptInput_IntegerList:
      /* Go through the list to find the min/max. */
      integerList = (const TA_IntegerList *)optInputParamInfo->dataSet;
      minInt = integerList->data[0].value;
      maxInt = integerList->data[0].value;
      for( i=0; i < integerList->nbElement; i++ )
      {
         minInt = min( integerList->data[i].value, minInt );
         maxInt = max( integerList->data[i].value, maxInt );
      }
      break;
   }

   switch( optInputParamInfo->type )
   {
   case TA_OptInput_RealList:
      fprintf( out, "   /* min/max are checked for %s_%d. */\n", name, paramNb );
   case TA_OptInput_RealRange:
      fprintf( out, "   if( %s_%d == TA_REAL_DEFAULT )\n", name, paramNb );
      fprintf( out, "      %s_%d = %s;\n", name, paramNb, doubleToStr(optInputParamInfo->defaultValue) );
      fprintf( out, "   else if( (%s_%d < %s) ||", name, paramNb, doubleToStr(minReal) );
      fprintf( out, " (%s_%d > %s) )\n", name, paramNb, doubleToStr(maxReal) );
              
      break;
   case TA_OptInput_IntegerRange:
      fprintf( out, "   /* min/max are checked for %s_%d. */\n", name, paramNb );
   case TA_OptInput_IntegerList:
      fprintf( out, "   if( (int)%s_%d == TA_INTEGER_DEFAULT )\n", name, paramNb );
      fprintf( out, "      %s_%d = %d;\n", name, paramNb, (int)optInputParamInfo->defaultValue );
      fprintf( out, "   else if( ((int)%s_%d < %d) || ((int)%s_%d > %d) )\n",
              name, paramNb, minInt,
              name, paramNb, maxInt );
      break;
   }

   fprintf( out, "      return TA_BAD_PARAM;\n\n" );
}


static int skipToGenCode( const char *dstName, FILE *out, FILE *templateFile )
{
   unsigned int headerWritten = 0;

   while( fgets( gTempBuf, 2048, templateFile ) )
   {
      if( strncmp( gTempBuf, "%%%GENCODE%%%", 13 ) == 0 )
      {
         headerWritten = 1;
         break;
      }
      if( fputs( gTempBuf, out ) == EOF )
      {
         printf( "Cannot write to [%s]\n", dstName );
         return -1;
      }
   }

   if( !headerWritten )
   {
      printf( "Line with string %%%%%%GENCODE%%%%%% Missing in [%s]", dstName );
      return -1;
   }

   return 0;
}

static void printFuncHeaderDoc( FILE *out,
                                const TA_FuncInfo *funcInfo,
                                const char *prefix )
{
   TA_RetCode retCode;
   unsigned int paramNb;
   const char *paramName;
   const char *defaultParamName;
   const TA_InputParameterInfo *inputParamInfo;
   const TA_OptInputParameterInfo *optInputParamInfo;
   const TA_OutputParameterInfo *outputParamInfo;
   int first;

   fprintf( out, "%sTA_%s - %s\n", prefix, funcInfo->name, funcInfo->hint );
   fprintf( out, prefix );

   fprintf( out, "\n%sInput  = ", prefix );
   for( paramNb=0; paramNb < funcInfo->nbInput; paramNb++ )
   {
      retCode = TA_SetInputParameterInfoPtr( funcInfo->handle,
                                             paramNb, &inputParamInfo );

      if( retCode != TA_SUCCESS )
      {
         printf( "[%s] invalid 'input' information\n", funcInfo->name );
         return;
      }

      switch( inputParamInfo->type )
      {
      case TA_Input_Price:
         first = 1;
         #define PRICE_PARAM(upperParam,lowerParam) \
         { \
            if( inputParamInfo->flags & TA_IN_PRICE_##upperParam ) \
            { \
               if( !first ) fprintf( out, ", " ); \
               fprintf( out, lowerParam ); \
               first = 0; \
            } \
         }

         PRICE_PARAM( OPEN,         "Open" );
         PRICE_PARAM( HIGH,         "High" );
         PRICE_PARAM( LOW,          "Low" );
         PRICE_PARAM( CLOSE,        "Close" );
         PRICE_PARAM( VOLUME,       "Volume" );
         PRICE_PARAM( OPENINTEREST, "OpenInterest" );
         PRICE_PARAM( TIMESTAMP,    "Timestamp" );

         #undef PRICE_PARAM

         break;
      case TA_Input_Integer:
         fprintf( out, "int" );
         break;
      case TA_Input_Real:
         fprintf( out, "double" );
         break;
/*      case TA_Input_Timestamp:
         fprintf( out, "TA_Timestamp" );
         break;*/
      }
      if( paramNb+1 == funcInfo->nbInput )
         fprintf( out, "\n" );
      else
         fprintf( out, ", " );
   }

   fprintf( out, "%sOutput = ", prefix );
   for( paramNb=0; paramNb < funcInfo->nbOutput; paramNb++ )
   {
      retCode = TA_SetOutputParameterInfoPtr( funcInfo->handle,
                                              paramNb, &outputParamInfo );

      if( retCode != TA_SUCCESS )
      {
         printf( "[%s] invalid 'output' information\n", funcInfo->name );
         return;
      }

      switch( outputParamInfo->type )
      {
      case TA_Output_Real:
         fprintf( out, "double" );
         break;
      case TA_Output_Integer:
         fprintf( out, "int" );
         break;
      }
      if( paramNb+1 == funcInfo->nbOutput )
         fprintf( out, "\n" );
      else
         fprintf( out, ", " );
   }

   if( funcInfo->nbOptInput != 0 )
   {
      fprintf( out, "%s\n", prefix );
      fprintf( out, "%sOptional Parameters\n", prefix );
      fprintf( out, "%s-------------------\n", prefix );
  
      for( paramNb=0; paramNb < funcInfo->nbOptInput; paramNb++ )
      {
         retCode = TA_SetOptInputParameterInfoPtr( funcInfo->handle,
                                                paramNb, &optInputParamInfo );

         if( retCode != TA_SUCCESS )
         {
            printf( "[%s] invalid 'optional input' information\n", funcInfo->name );
            return;
         }

         paramName = optInputParamInfo->paramName;

         switch( optInputParamInfo->type )
         {
         case TA_OptInput_RealRange:
         case TA_OptInput_RealList:
            defaultParamName = "optInReal";
            break;
         case TA_OptInput_IntegerRange:
         case TA_OptInput_IntegerList:
            defaultParamName = "optInInteger";
            break;
         default:
            if( !paramName )
               paramName = "optInParam";
            printf( "[%s,%s_%d] invalid 'optional input' type(%d)\n",
                    funcInfo->name, paramName, paramNb,
                    optInputParamInfo->type );
            return;
         }

         if( !paramName )
            paramName = defaultParamName;

         fprintf( out, "%s%s_%d:", prefix, paramName, paramNb );
         switch( optInputParamInfo->type )
         {
         case TA_OptInput_RealRange:
               if( ((TA_RealRange *)(optInputParamInfo->dataSet))->min == TA_REAL_MIN )
                  fprintf( out, "(From TA_REAL_MIN" );
               else
               {
                  fprintf( out, "(From %.*g",
                           ((TA_RealRange *)(optInputParamInfo->dataSet))->precision,
                           ((TA_RealRange *)(optInputParamInfo->dataSet))->min );
               }

               if( ((TA_RealRange *)(optInputParamInfo->dataSet))->max == TA_REAL_MAX )
                  fprintf( out, " to TA_REAL_MAX)\n" );
               else
               {
                  fprintf( out, " to %.*g%s)\n", 
                          ((TA_RealRange *)(optInputParamInfo->dataSet))->precision,
                          ((TA_RealRange *)(optInputParamInfo->dataSet))->max,
                          optInputParamInfo->flags & TA_OPTIN_IS_PERCENT? " %":"" );
               }
               break;
         case TA_OptInput_IntegerRange:
               if( ((TA_IntegerRange *)(optInputParamInfo->dataSet))->min == TA_INTEGER_MIN )
                  fprintf( out, "(From TA_INTEGER_MIN" );
               else
               {
                  fprintf( out, "(From %d",
                           ((TA_IntegerRange *)(optInputParamInfo->dataSet))->min );
               }

               if( ((TA_IntegerRange *)(optInputParamInfo->dataSet))->max == TA_INTEGER_MAX )
                  fprintf( out, " to TA_INTEGER_MAX)\n" );
               else
               {
                  fprintf( out, " to %d)\n", 
                          ((TA_IntegerRange *)(optInputParamInfo->dataSet))->max );
               }
               break;
         default:
             fprintf( out, "\n" );
         }
         fprintf( out, "%s   %s\n", prefix, optInputParamInfo->hint );
         fprintf( out, "%s\n", prefix );
      }
   }

   fprintf( out, "%s\n", prefix );
}

static int addUnstablePeriodEnum( FILE *out )
{
   TA_RetCode retCode;
   unsigned int id;
   
   fprintf( out, "\n" );
   fprintf( out, "#if defined( _MANAGED )\n");
   fprintf( out, "public __value enum TA_FuncUnstId\n");
   fprintf( out, "#else\n");
   fprintf( out, "typedef enum\n");
   fprintf( out, "#endif\n");
   fprintf( out, "{\n");

   /* Enumerate the function having an "unstable period". Give
    * to each an unique identifier.
    */
   id = 1;
   retCode = TA_ForEachFunc( doForEachUnstableFunction, &id );

   fprintf( out, "               TA_FUNC_UNST_ALL,\n");
   fprintf( out, "               TA_FUNC_UNST_NONE=-1\n" );
   fprintf( out, "#if defined( _MANAGED )\n");
   fprintf( out, "};\n");
   fprintf( out, "#else\n");
   fprintf( out, "} TA_FuncUnstId;\n");
   fprintf( out, "#endif\n");

   if( retCode != TA_SUCCESS )
      return -1;

   return 0;
}

static int gen_retcode( void )
{
   FileHandle *inHdr;
   char *ptr1, *ptr2;
   int step1Done;
   int retValue;
   TA_RetCode retCodeValue;
   char *retCodeEnum, *retCodeInfo;

   retValue = -1;

   /* Create "ta_retcode.c" */
   gOutRetCode_C = fileOpen( "..\\src\\ta_common\\ta_retcode.c",
                             "..\\src\\ta_abstract\\templates\\ta_retcode.c.template",
                             FILE_WRITE );

   if( gOutRetCode_C == NULL )
   {
      printf( "\nCannot access [%s]\n", gToOpen );
      return -1;
   }

   /* Create "ta_retcode.csv" */
   gOutRetCode_CSV = fileOpen( "..\\src\\ta_common\\ta_retcode.csv", NULL, FILE_WRITE );

   if( gOutRetCode_CSV == NULL )
   {
      fileClose( gOutRetCode_C );
      printf( "\nCannot access [%s]\n", gToOpen );
      return -1;
   }

   inHdr = fileOpen( "..\\include\\ta_defs.h", NULL, FILE_READ );
   if( inHdr == NULL )
   {
      fileClose( gOutRetCode_C );
      fileClose( gOutRetCode_CSV );
      printf( "\nCannot access [%s]\n", gToOpen );
      return -1;
   }

   step1Done = 0;
   while( fgets( gTempBuf, sizeof( gTempBuf ), inHdr->file ) )
   {
      if( !step1Done )
      {
         if( stricstr( gTempBuf, "TA-LIB Error Code" ) != NULL )
            step1Done = 1;
      }
      else
      {
         if( stricstr( gTempBuf, "TA_UNKNOWN_ERR" ) != NULL )
         {
            retValue = 0;
            goto gen_retcode_exit;
         }

         ptr1 = stricstr( gTempBuf, "/*" );
         ptr2 = stricstr( gTempBuf, "/***" );
         if( ptr1 && !ptr2 )
         {
            ptr1 += 2;
            retCodeValue = atoi(ptr1);
            ptr1 = stricstr( ptr1, "TA_" );
            if( !ptr1 )
            {
               printf( "Can't find TA_" );
               goto gen_retcode_exit;
            }
            retCodeEnum = ptr1;

            retCodeInfo = NULL;
            while( isdigit(*ptr1) || isalpha(*ptr1) || *ptr1 == '_' )
               ptr1++;
            if( *ptr1 != '\0' )
            {
               *ptr1 = '\0';
               ptr1++;
               if( *ptr1 != '\0' )
               {
                  retCodeInfo = stricstr( ptr1, "/* " );
                  if( retCodeInfo )
                  {
                     retCodeInfo += 3;
                     ptr1 = stricstr( retCodeInfo, "*/" );
                     if( ptr1 == NULL )
                        retCodeInfo = NULL;
                     else
                        *ptr1 = '\0';
                  }
               }                  
            }

            if( !retCodeInfo )
               retCodeInfo = "No Info";

            strcpy( gTempBuf, retCodeEnum );
            ptr1 = trim( gTempBuf );
            fprintf( gOutRetCode_C->file, "         {%d,\"%s\",", retCodeValue, ptr1 );                     
            fprintf( gOutRetCode_CSV->file, "%d,%s", retCodeValue, ptr1 );
            strcpy( gTempBuf, retCodeInfo );
            ptr1 = trim( gTempBuf );
            fprintf( gOutRetCode_C->file, "\"%s\"},\n", ptr1 );
            fprintf( gOutRetCode_CSV->file, ",%s\n", ptr1 );
         }
      }
   }

gen_retcode_exit:
   fileClose( inHdr );
   fileClose( gOutRetCode_C );

   return retValue; /* Success. */
}

const char *doubleToStr( double value )
{
   int length;
   int i, outIdx;
   sprintf( gTempDoubleToStr, "%e", value );

   /* Remove extra "0" added by Microsoft in the
    * exponential part.
    */
   length = strlen( gTempDoubleToStr );
   outIdx = 0;
   for( i=0; i < length; i++ )
   {
      /* Will skip two leading zero in the exponent */
      if( (i >= 2) &&
          (toupper(gTempDoubleToStr[i-2]) == 'E') &&
          ((gTempDoubleToStr[i-1] == '+')||(gTempDoubleToStr[i-1] == '-')) &&
          (gTempDoubleToStr[i] == '0') &&
          (gTempDoubleToStr[i+1] == '0') &&
          (isdigit(gTempDoubleToStr[i+2])) )
      {
         i++;
         continue;
      }

      /* Will skip one leading zero in the exponent */
      if( (i >= 2) &&
          (toupper(gTempDoubleToStr[i-2]) == 'E') &&
          ((gTempDoubleToStr[i-1] == '+')||(gTempDoubleToStr[i-1] == '-')) &&
          (gTempDoubleToStr[i] == '0') &&
          (isdigit(gTempDoubleToStr[i+1])))
      {
         continue;
      }

      gTempDoubleToStr[outIdx++] = gTempDoubleToStr[i];
   }
   gTempDoubleToStr[outIdx] = '\0';

   return gTempDoubleToStr;
}

static void printExcelGlueCode( FILE *out, const TA_FuncInfo *funcInfo )
{
   /*fprintf( out, "#include \"ta_%s.c\"\n", funcInfo->name );
   fprintf( out, "#include \"ta_%s_frame.c\"\n", funcInfo->name );   */
   int nbParam;
   unsigned int i;
   TA_RetCode retCode;
   const TA_InputParameterInfo *inputParamInfo;

   nbParam = funcInfo->nbOptInput;

   for( i=0; i < funcInfo->nbInput; i++ )
   {
      retCode = TA_SetInputParameterInfoPtr( funcInfo->handle,
                                               i, &inputParamInfo );

      if( retCode != TA_SUCCESS )
      {
         printf( "[%s] invalid 'input' information (%d,%d)\n", funcInfo->name, i, nbParam );
         return;
      }

      if( inputParamInfo->type != TA_Input_Price )
         nbParam++;
      else
      {
         if( inputParamInfo->flags & TA_IN_PRICE_TIMESTAMP )
            nbParam++;
         if( inputParamInfo->flags & TA_IN_PRICE_OPEN )
            nbParam++;
         if( inputParamInfo->flags & TA_IN_PRICE_HIGH )
            nbParam++;
         if( inputParamInfo->flags & TA_IN_PRICE_LOW )
            nbParam++;
         if( inputParamInfo->flags & TA_IN_PRICE_CLOSE )
            nbParam++;
         if( inputParamInfo->flags & TA_IN_PRICE_VOLUME )
            nbParam++;
         if( inputParamInfo->flags & TA_IN_PRICE_OPENINTEREST )         
            nbParam++;
      }
   }

   fprintf( out, "EXCEL_GLUE_CODE_WITH_%d_PARAM(%s)\n", 
           nbParam,
           funcInfo->name );
}

static void addFuncEnumeration( FILE *out )
{
   TA_IntegerList *intList;
   unsigned int j;

   /* Add the TA_MAType enumeration */
   intList = (TA_IntegerList *)TA_DEF_UI_MA_Method.dataSet;
   fprintf( out, "\n" );
   fprintf( out, "#if defined( _MANAGED )\n" );
   fprintf( out, "public __value enum TA_MAType\n" );
   fprintf( out, "#else\n" );
   fprintf( out, "typedef enum\n" );
   fprintf( out, "#endif\n" );
   fprintf( out, "{\n" );
   for( j=0; j < intList->nbElement; j++ )
   {
      strcpy( gTempBuf, intList->data[j].string );
      strconvch( gTempBuf, ' ', '_' );
      trim( gTempBuf );
      strupc( gTempBuf );
      fprintf( out, "   TA_MAType_%-10s=%d%s", gTempBuf, intList->data[j].value,
                     j < (intList->nbElement)-1?",\n":"\n");
   }

   fprintf( out, "#if defined( _MANAGED )\n" );
   fprintf( out, "};\n" );
   fprintf( out, "#else\n" );
   fprintf( out, "} TA_MAType;\n" );
   fprintf( out, "#endif\n" );
}
