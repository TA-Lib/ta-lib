/* TA-LIB Copyright (c) 1999-2003, Mario Fortier
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
 *  112400 MF   First version.
 *  052403 MF   Many modifications related to generate code that works
 *              with the windows .NET Managed C++ compiler.
 *  092103 MF   Now touch files only when there is really a change.
 *  101303 MF   Remove underscore from names.
 */

/* Description:
 *       Generates a lot of source code. Run "gen_code ?" for
 *       the list of file.
 *
 *       This utility have no used for an end-user of the TA-LIB.
 *       It is useful only to people integrating new TA functions
 *       in TA-Lib.
 *
 * Note: All directory in this code is relative to the 'bin'
 *       directory. So you must run the executable from ta-lib/c/bin.
 *       
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "ta_common.h"
#include "ta_abstract.h"
#include "ta_system.h"
#include "sfl.h"

#define BUFFER_SIZE 8192

#define FILE_WRITE            0
#define WRITE_ON_CHANGE_ONLY  0

#define FILE_READ     0x00000001
#define WRITE_ALWAYS  0x00000002
#define SORT_OUTPUT   0x00000004

#ifndef min
   #define min(a, b)  (((a) < (b)) ? (a) : (b))
#endif

#ifndef max
   #define max(a, b)  (((a) > (b)) ? (a) : (b))
#endif

typedef struct
{
   FILE *file;
   FILE *fileTarget;
   FILE *templateFile;
   char f1_name[BUFFER_SIZE];
   char f2_name[BUFFER_SIZE];
   
   int flags;
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
FileHandle *gOutDefs_H;        /* For "ta_defs.h" */
FileHandle *gOutProjFile;      /* For .NET project file */
FileHandle *gOutDotNet_H;      /* For .NET interface file */

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
                       unsigned int prototype,             /* Boolean */
                       unsigned int frame,                 /* Boolean */
                       unsigned int semiColonNeeded,       /* Boolean */
                       unsigned int validationCode,        /* Boolean */
                       unsigned int lookbackSignature,     /* Boolean */
                       unsigned int managedCPPCode,        /* Boolean */
                       unsigned int managedCPPDeclaration, /* Boolean */
                       unsigned int inputIsSinglePrecision /* Boolean */
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
static int createProjTemplate( FileHandle *in, FileHandle *out );

static void writeFuncFile( const TA_FuncInfo *funcInfo );
static void doFuncFile( const TA_FuncInfo *funcInfo );
static void printOptInputValidation( FILE *out,
                                     const char *name,                                     
                                     const TA_OptInputParameterInfo *optInputParamInfo );
static int skipToGenCode( const char *dstName, FILE *out, FILE *templateFile );
static void printDefines( FILE *out, const TA_FuncInfo *funcInfo );

static void printFuncHeaderDoc( FILE *out,
                                const TA_FuncInfo *funcInfo,
                                const char *prefix );


static void addFuncEnumeration( FILE *out );

static void extractTALogic( FILE *inFile, FILE *outFile );

static void sortFile( const char *file );

/* Return 1 on success */
static int copyFile( const char *src, const char *dest );

/* Return 1 when identical */
static int areFileSame( const char *file1, const char *file2 );

char gToOpen[BUFFER_SIZE];
char gTempBuf[BUFFER_SIZE];
char gTempBuf2[BUFFER_SIZE];
char gTempBuf3[BUFFER_SIZE];
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

/* Set this variable to 1 whenever you wish to output a
 * prefix to all generated line.
 */
int genPrefix = 0;

void print( FILE *out, const char *text, ... )
{
   va_list arglist;
   char buff[1024];
   memset(buff,0,sizeof(buff));

   va_start(arglist,text);
   vsprintf(buff,text,arglist);
   va_end(arglist);

   if( genPrefix )
      fprintf( out, "/* Generated */ %s", buff );
   else
      fprintf( out, "%s", buff );
}

static void printIndent( FILE *out, unsigned int indent )
{
   unsigned int i;
   
   if( genPrefix )
      fprintf( out, "/* Generated */ " );

   for( i=0; i < indent; i++ )
   {
      fprintf( out, " " );
   }
}


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
         printf( "     8) ta-lib/dotnet/src/Core/TA-Lib-Core.vcproj\n" );
         printf( "     9) ta-lib/dotnet/src/Core/Core.h\n" );

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
 * Another advantage to use fileOpen and fileClose is that
 * the writing to the file is done "silently" in a temporary
 * file and the target file is touch only if there was actually
 * a modification to it.
 *
 * On failure, simply exit the software.
 */
static void init_gToOpen( const char *filePath, const char *suffix )
{
   int sepChar;
   char *ptr;

   sepChar = TA_SeparatorASCII();

   strcpy( gToOpen, filePath );
   if( suffix )
      strcat( gToOpen, suffix );

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
                              int flags )
{
   FileHandle *retValue;

   if( (fileToOpen == NULL) ||
       ((flags&FILE_READ) && (templateFile != NULL)) )
   {
      printf( "Internal error line %d", __LINE__ );
      return (FileHandle *)NULL;
   }

   retValue = malloc( sizeof(FileHandle) );
   memset( retValue, 0, sizeof(FileHandle) );

   retValue->flags = flags;

   init_gToOpen( fileToOpen, NULL );
   strcpy( retValue->f1_name, gToOpen );
   
   /* First let's try to open the file. Might fail when
    * for writing but that is ok. (the file might not exist).
    */

   if( flags&FILE_READ )
   {      
      retValue->file = fopen( gToOpen, "r" );
      if( retValue->file == NULL )
      {
         free( retValue );
         return (FileHandle *)NULL;
      }
   }
   else if( flags&WRITE_ALWAYS )
   {
      retValue->file = fopen( gToOpen, "w" );
      if( retValue->file == NULL )
      {
         free( retValue );
         return (FileHandle *)NULL;
      }
   }
   else
   {
      retValue->file = fopen( gToOpen, "r" );

      if( retValue->file )
      {
         /* Move pointer to fileTarget.  The file
          * ptr will become the temporary file who
          * is going to be truly write enabled.
          */
         retValue->fileTarget = retValue->file;
         retValue->file = NULL;

         init_gToOpen( fileToOpen, ".tmp" );
         strcpy( retValue->f2_name, gToOpen );
         retValue->file = fopen( gToOpen, "w" );
         if( !retValue->file )
         {
            fclose( retValue->fileTarget );
            free( retValue );
            return (FileHandle *)NULL;
         }
       }
       else
       {
         /* File does not exist, directly open for write
          * no temporary will be used.
          */
         retValue->file = fopen( gToOpen, "w" );

         if( retValue->file == NULL )
         {
            if(retValue->fileTarget)   fclose( retValue->fileTarget );
            free( retValue );
            return (FileHandle *)NULL;
         }
      }
   }

   if( !(flags&FILE_READ) )
   {
      /* Handle the template. */
      if( templateFile )
      {
         init_gToOpen( templateFile, NULL );
         retValue->templateFile = fopen( gToOpen, "r" );
         if( retValue->templateFile == NULL )
         {
            if(retValue->fileTarget)   fclose( retValue->fileTarget );
            if(retValue->file)         fclose( retValue->file );
            if(retValue->templateFile) fclose( retValue->templateFile );
            free( retValue );
            printf( "\nCannot open template [%s]\n", gToOpen );
            return (FileHandle *)NULL;
         }

         /* Copy the header part of the template. */
         if( skipToGenCode( fileToOpen, retValue->file, retValue->templateFile ) != 0 )
         {
            if(retValue->fileTarget)   fclose( retValue->fileTarget );
            if(retValue->file)         fclose( retValue->file );
            if(retValue->templateFile) fclose( retValue->templateFile );
            free( retValue );
            retValue = NULL;
         }
      }
   }

   return retValue;
}

static void fileClose( FileHandle *handle )
{
   /* Write remaining template info. */
   if( handle->templateFile )
   {
      while( fgets( gTempBuf, BUFFER_SIZE, handle->templateFile ) != NULL )
      {
         if( fputs( gTempBuf, handle->file ) == EOF )
         {
            printf( "Cannot write to output file! Disk Full? " );
            break;
         }
      }

      #ifdef _MSVC
         /* Make sure the last line of the output 
          * finish with a carriage return. This may
          * avoid warning from some compilers.
          */
         if( gTempBuf[0] != '\n' )
	     {
            fprintf( handle->file, "\n" );
	     }
      #endif

      fclose( handle->templateFile );
   }

   if(handle->fileTarget)   fclose( handle->fileTarget );
   if(handle->templateFile) fclose( handle->templateFile );
   if(handle->file)         fclose( handle->file );

   if( !(handle->flags&FILE_READ) && !(handle->flags&WRITE_ALWAYS) )
   {
      if( handle->flags&SORT_OUTPUT )
         sortFile(handle->f2_name);

      if( !areFileSame( handle->f1_name, handle->f2_name ) )
         copyFile( handle->f2_name, handle->f1_name );

      file_delete( handle->f2_name );      
   }
   
   free( handle );
}

static void fileDelete( const char *fileToDelete )
{
   init_gToOpen( fileToDelete, NULL );
   file_delete( gToOpen );
}

static int genCode(int argc, char* argv[])
{
   TA_RetCode retCode;
   unsigned int nbGroup;
   FileHandle *tempFile;

   (void)argc; /* Get ride of compiler warning */
   (void)argv; /* Get ride of compiler warning */

   /* Create .NET project files template */
   #define FILE_NET_PROJ     "..\\..\\dotnet\\src\\Core\\TA-Lib-Core.vcproj"
   #define FILE_NET_PROJ_TMP "..\\temp\\dotnetproj.tmp"
   gOutProjFile = fileOpen( FILE_NET_PROJ, NULL, FILE_READ|WRITE_ON_CHANGE_ONLY );
   if( gOutProjFile == NULL )   
   {
      printf( "\nCannot access [%s]\n", gToOpen );
      return -1;
   }
   tempFile = fileOpen( FILE_NET_PROJ_TMP, NULL, FILE_WRITE|WRITE_ALWAYS );
   if( tempFile == NULL )
   {
      printf( "Cannot create temporary .NET project file!\n" );
      return -1;
   }
   if( createProjTemplate( gOutProjFile, tempFile ) != 0 )
   {
      printf( "Failed to parse and write the temporary .NET project file!\n" );
      return -1;
   }
   fileClose(gOutProjFile);
   fileClose(tempFile);

   /* Create the .NET interface file template */
   #define FILE_NET_HEADER     "..\\..\\dotnet\\src\\Core\\Core.h"
   #define FILE_NET_HEADER_TMP "..\\temp\\dotneth.tmp"
   gOutDotNet_H = fileOpen( FILE_NET_HEADER, NULL, FILE_READ|WRITE_ON_CHANGE_ONLY );
   if( gOutDotNet_H == NULL )   
   {
      printf( "\nCannot access [%s]\n", gToOpen );
      return -1;
   }
   tempFile = fileOpen( FILE_NET_HEADER_TMP, NULL, FILE_WRITE|WRITE_ALWAYS );
   if( tempFile == NULL )
   {
      printf( "Cannot create temporary .NET header file!\n" );
      return -1;
   }
   if( createTemplate( gOutDotNet_H, tempFile ) != 0 )
   {
      printf( "Failed to parse and write the temporary .NET header file!\n" );
      return -1;
   }
   fileClose(gOutDotNet_H);
   fileClose(tempFile);

   /* Create ta_retcode.c */
   if( gen_retcode() != 0 )
   {
      printf( "\nCannot generate src/ta_common/ta_retcode.c\n" );
      return -1;
   }

   /* Create "ta_func.h" */
   gOutFunc_H = fileOpen( "..\\include\\ta_func.h",
                          "..\\src\\ta_abstract\\templates\\ta_func.h.template",
                          FILE_WRITE|WRITE_ON_CHANGE_ONLY );

   if( gOutFunc_H == NULL )
   {
      printf( "\nCannot access [%s]\n", gToOpen );
      return -1;
   }

   /* Create the "func_list.txt" */
   gOutFuncList_TXT = fileOpen( "..\\include\\func_list.txt",
                                NULL,
                                FILE_WRITE|WRITE_ON_CHANGE_ONLY|SORT_OUTPUT );

   if( gOutFuncList_TXT == NULL )
   {
      printf( "\nCannot access [%s]\n", gToOpen );
      return -1;
   }


   /* Create the "ta_frame.h" */
   gOutFrame_H = fileOpen( "..\\src\\ta_abstract\\frames\\ta_frame.h",
                           "..\\src\\ta_abstract\\templates\\ta_frame.h.template",
                           FILE_WRITE|WRITE_ON_CHANGE_ONLY );

   if( gOutFrame_H == NULL )
   {
      printf( "\nCannot access [%s]\n", gToOpen );
      return -1;
   }

   /* Create the "ta_frame.c" */
   gOutFrame_C = fileOpen( "..\\src\\ta_abstract\\frames\\ta_frame.c",
                           "..\\src\\ta_abstract\\templates\\ta_frame.c.template",
                           FILE_WRITE|WRITE_ON_CHANGE_ONLY );

   if( gOutFrame_C == NULL )
   {
      printf( "\nCannot access [%s]\n", gToOpen );
      return -1;
   }

   /* Create "excel_glue.c" */
   gOutExcelGlue_C = fileOpen( "..\\src\\ta_abstract\\excel_glue.c",
                           "..\\src\\ta_abstract\\templates\\excel_glue.c.template",
                           FILE_WRITE|WRITE_ON_CHANGE_ONLY );

   if( gOutExcelGlue_C == NULL )
   {
      printf( "\nCannot access [%s]\n", gToOpen );
      return -1;
   }

   /* Re-open the .NET project template. */
   gOutProjFile = fileOpen( FILE_NET_PROJ, FILE_NET_PROJ_TMP, FILE_WRITE|WRITE_ON_CHANGE_ONLY );
   if( gOutProjFile == NULL )
   {
      printf( "Cannot update [%s]\n", FILE_NET_PROJ );
      return -1;
   }

   /* Re-open the .NET interface template. */
   gOutDotNet_H = fileOpen( FILE_NET_HEADER, FILE_NET_HEADER_TMP, FILE_WRITE|WRITE_ON_CHANGE_ONLY );
   if( gOutDotNet_H == NULL )
   {
      printf( "Cannot update [%s]\n", FILE_NET_HEADER );
      return -1;
   }

   /* Process each function. */
   retCode = TA_ForEachFunc( doForEachFunction, NULL );

   /* Close all files who were updated with the list of TA functions. */
   fileClose( gOutDotNet_H );
   fileClose( gOutProjFile );
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
   genPrefix = 1;
   gOutGroupIdx_C = fileOpen( "..\\src\\ta_abstract\\ta_group_idx.c",
                              "..\\src\\ta_abstract\\templates\\ta_group_idx.c.template",
                              FILE_WRITE|WRITE_ON_CHANGE_ONLY );

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

   print( gOutGroupIdx_C->file, "const TA_FuncDef **TA_PerGroupFuncDef[%d] = {\n", nbGroup );
   forEachGroup( printGroupListAddress, NULL );
   print( gOutGroupIdx_C->file, "};\n\n" );

   print( gOutGroupIdx_C->file, "const unsigned int TA_PerGroupSize[%d] = {\n", nbGroup );
   forEachGroup( printGroupSize, NULL );
   print( gOutGroupIdx_C->file, "};\n\n" );

   print( gOutGroupIdx_C->file, "const unsigned int TA_TotalNbFunction =\n" );
   forEachGroup( printGroupSizeAddition, NULL );

   fileClose( gOutGroupIdx_C );
   genPrefix = 0;

   /* Update "ta_defs.h" */
   doDefsFile();

   /* Remove some temporary files */
   fileDelete( FILE_NET_PROJ_TMP   );
   fileDelete( FILE_NET_HEADER_TMP );

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
   genPrefix = 0;
   fprintf( gOutFuncList_TXT->file, "%-20s%s\n", funcInfo->name, funcInfo->hint );
  
   fprintf( gOutFunc_H->file, "\n" );

   if( (prevGroup == NULL) || (prevGroup != funcInfo->group) )
   {
      printf( "Processing Group [%s]\n", funcInfo->group );
      fprintf( gOutFunc_H->file, "\n/******************************************\n" );
      fprintf( gOutFunc_H->file, " * Group: [%s]\n", funcInfo->group );
      fprintf( gOutFunc_H->file, " ******************************************/\n\n" );

      prevGroup = funcInfo->group;
   }

   /* printf( "Processing Func  [TA_%s]\n", funcInfo->name ); */

   fprintf( gOutFunc_H->file, "/*\n" );
   printFuncHeaderDoc( gOutFunc_H->file, funcInfo, " * " );
   fprintf( gOutFunc_H->file, " */\n" );

   /* Generate the defines corresponding to this function. */
   printDefines( gOutFunc_H->file, funcInfo );


   /* Generate the function prototype. */
   printFunc( gOutFunc_H->file, NULL, funcInfo, 1, 0, 1, 0, 0, 0, 0, 0 );
   fprintf( gOutFunc_H->file, "\n" );

   printFunc( gOutFunc_H->file, NULL, funcInfo, 1, 0, 1, 0, 0, 0, 0, 1 );
   fprintf( gOutFunc_H->file, "\n" );

   /* Generate the corresponding lookback function prototype. */
   printFunc( gOutFunc_H->file, NULL, funcInfo, 1, 0, 1, 0, 1, 0, 0, 0 );

   /* Generate the excel glue code */
   printExcelGlueCode( gOutExcelGlue_C->file, funcInfo );

   /* Create the frame definition (ta_frame.c) and declaration (ta_frame.h) */
   genPrefix = 1;
   printFrameHeader( gOutFrame_H->file, funcInfo );
   fprintf( gOutFrame_H->file, ";\n\n" );
   printCallFrame( gOutFrame_C->file, funcInfo );

   /* Add the entry in the .NET project file */
   fprintf( gOutProjFile->file, "				<File\n" );
   fprintf( gOutProjFile->file, "					RelativePath=\"..\\..\\..\\c\\src\\ta_func\\ta_%s.c\">\n", funcInfo->name );
   fprintf( gOutProjFile->file, "					<FileConfiguration\n" );
   fprintf( gOutProjFile->file, "						Name=\"Debug|Win32\">\n" );
   fprintf( gOutProjFile->file, "						<Tool\n" );
   fprintf( gOutProjFile->file, "							Name=\"VCCLCompilerTool\"\n" );
   fprintf( gOutProjFile->file, "							AdditionalIncludeDirectories=\"\"\n" );
   fprintf( gOutProjFile->file, "							UsePrecompiledHeader=\"0\"\n" );
   fprintf( gOutProjFile->file, "							CompileAs=\"2\"/>\n" );
   fprintf( gOutProjFile->file, "					</FileConfiguration>\n" );
   fprintf( gOutProjFile->file, "					<FileConfiguration\n" );
   fprintf( gOutProjFile->file, "						Name=\"Release|Win32\">\n" );
   fprintf( gOutProjFile->file, "						<Tool\n" );
   fprintf( gOutProjFile->file, "							Name=\"VCCLCompilerTool\"\n" );
   fprintf( gOutProjFile->file, "							AdditionalIncludeDirectories=\"\"\n" );
   fprintf( gOutProjFile->file, "							UsePrecompiledHeader=\"0\"\n" );
   fprintf( gOutProjFile->file, "							CompileAs=\"2\"/>\n" );
   fprintf( gOutProjFile->file, "					</FileConfiguration>\n" );
   fprintf( gOutProjFile->file, "				</File>\n" );

   /* Generate the functions declaration for the .NET interface. */
   printFunc( gOutDotNet_H->file, NULL, funcInfo, 1, 0, 1, 0, 1, 1, 1, 0 );
   printFunc( gOutDotNet_H->file, NULL, funcInfo, 1, 0, 1, 0, 0, 1, 1, 0 );
   printFunc( gOutDotNet_H->file, NULL, funcInfo, 1, 0, 1, 0, 0, 1, 1, 1 );
   fprintf( gOutDotNet_H->file, "\n" );
   fprintf( gOutDotNet_H->file, "         #define TA_%s Core::%s\n", funcInfo->name, funcInfo->name );
   fprintf( gOutDotNet_H->file, "         #define TA_%s_Lookback Core::%s_Lookback\n\n", funcInfo->name, funcInfo->name );

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
               fprintf( out, "\n/* TA_%s: Optional Parameter %s */\n",
                        funcInfo->name, paramName );
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
            fprintf( out, "\n/* TA_%s: Optional Parameter %s */\n",
                     funcInfo->name, paramName );

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
                       unsigned int managedCPPCode, /* Boolean */
                       unsigned int managedCPPDeclaration, /* Boolean */
                       unsigned int inputIsSinglePrecision
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
   int excludeFromManaged;

   const char *inputPrecisionType;
   const char *inputPrecisionConstType;

   if( inputIsSinglePrecision )
   {
      inputPrecisionType = "float";
      inputPrecisionConstType = "const float";
   }
   else
   {
      inputPrecisionType = "double";
      inputPrecisionConstType = "const double";
   }

   if( managedCPPCode )
   {
      inputDoubleArrayType  = inputPrecisionType;
      inputIntArrayType     = "int";
      outputIntParam        = "[OutAttribute]Int32";
      arrayBracket          = " __gc []";
   }
   else
   {
      inputDoubleArrayType  = inputPrecisionConstType;
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
         if( managedCPPCode )
         {       
            sprintf( gTempBuf, "%s%sint %s%s_Lookback( ",
                     prefix? prefix:"",
                     managedCPPDeclaration? "         static ":"",
                     managedCPPDeclaration? "":"Core::",
                     funcInfo->name );
         }
         else
         {
            sprintf( gTempBuf, "%sint TA_%s_Lookback( ",
                     prefix? prefix:"",
                     funcInfo->name );
         }
         print( out, gTempBuf );
         indent = strlen(gTempBuf) - 2;
      }
      else
      {
         if( managedCPPCode )
         {
            sprintf( gTempBuf, "%s%senum %sTA_RetCode %s%s( int    startIdx,\n",
                     prefix? prefix:"",
                     managedCPPDeclaration? "         static ":"",
                     managedCPPDeclaration? "":"Core::",
                     managedCPPDeclaration? "":"Core::",
                     funcInfo->name );
         }
         else
         {
            if( inputIsSinglePrecision )
               sprintf( gTempBuf, "%sTA_RetCode TA_S_%s( int    startIdx,\n",
                        prefix? prefix:"",
                        funcInfo->name );
            else  
               sprintf( gTempBuf, "%sTA_RetCode TA_%s( int    startIdx,\n",
                        prefix? prefix:"",
                        funcInfo->name );
         }
         print( out, gTempBuf );
         indent = strlen(gTempBuf) - 17;
         printIndent( out, indent );
         fprintf( out, "int    endIdx,\n" );
      }
   }
   else if( frame )
   {
      print( out, "%sTA_%s(\n",
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
                  fprintf( out, "!inTimestamp%s", k != j? "||":")");
               }

               if( inputParamInfo->flags & TA_IN_PRICE_OPEN )
               {
                  k++;
                  fprintf( out, "!inOpen%s", k != j? "||":")");
               }
               
               if( inputParamInfo->flags & TA_IN_PRICE_HIGH )
               {
                  k++;
                  fprintf( out, "!inHigh%s", k != j? "||":")");
               }

               if( inputParamInfo->flags & TA_IN_PRICE_LOW )
               {
                  k++;
                  fprintf( out, "!inLow%s", k != j? "||":")");
               }

               if( inputParamInfo->flags & TA_IN_PRICE_CLOSE )
               {
                  k++;
                  fprintf( out, "!inClose%s", k != j? "||":")");
               }

               if( inputParamInfo->flags & TA_IN_PRICE_VOLUME )
               {
                  k++;
                  fprintf( out, "!inVolume%s", k != j? "||":")");
               }

               if( inputParamInfo->flags & TA_IN_PRICE_OPENINTEREST )
               {
                  k++;
                  fprintf( out, "!inOpenInterest%s", k != j? "||":")");
               }

               fprintf( out, "\n" );
               printIndent( out, indent );
               fprintf( out, "   return TA_BAD_PARAM;\n" );
               print( out, "\n" );
            }
            else
            {
               if( inputParamInfo->flags & TA_IN_PRICE_TIMESTAMP )
               {
                  printIndent( out, indent );
                  if( frame )
                     fprintf( out, "params->in[%d].data.inPrice.timestamp, /*", paramNb );
                  fprintf( out, "%-*s %s%s",
                         prototype? 12 : 0,
                         prototype? "const TA_Timestamp" : "",                           
                         "inTimestamp",
                         prototype? arrayBracket : "" );
                  fprintf( out, "%s\n", frame? " */":"," );
               }

               if( inputParamInfo->flags & TA_IN_PRICE_OPEN )
               {
                  printIndent( out, indent );
                  if( frame )
                     fprintf( out, "params->in[%d].data.inPrice.open, /*", paramNb );
                  fprintf( out, "%-*s %s%s",
                         prototype? 12 : 0,
                         prototype? inputDoubleArrayType : "",
                         "inOpen",
                         prototype? arrayBracket : "" );
                  fprintf( out, "%s\n", frame? " */":"," );
               }

               if( inputParamInfo->flags & TA_IN_PRICE_HIGH )
               {
                  printIndent( out, indent );
                  if( frame )
                     fprintf( out, "params->in[%d].data.inPrice.high, /*", paramNb );
                  fprintf( out, "%-*s %s%s",
                         prototype? 12 : 0,
                         prototype? inputDoubleArrayType : "",                           
                         "inHigh",
                         prototype? arrayBracket : "" );
                  fprintf( out, "%s\n", frame? " */":"," );
               }

               if( inputParamInfo->flags & TA_IN_PRICE_LOW )
               {
                  printIndent( out, indent );
                  if( frame )
                     fprintf( out, "params->in[%d].data.inPrice.low, /*", paramNb );
                  fprintf( out, "%-*s %s%s",
                         prototype? 12 : 0,
                         prototype? inputDoubleArrayType : "",
                         "inLow",
                         prototype? arrayBracket : "" );
                  fprintf( out, "%s\n", frame? " */":"," );
               }

               if( inputParamInfo->flags & TA_IN_PRICE_CLOSE )
               {
                  printIndent( out, indent );
                  if( frame )
                     fprintf( out, "params->in[%d].data.inPrice.close, /*", paramNb );
                  fprintf( out, "%-*s %s%s",
                         prototype? 12 : 0,
                         prototype? inputDoubleArrayType : "",                           
                         "inClose",
                         prototype? arrayBracket : "" );
                  fprintf( out, "%s\n", frame? " */":"," );
               }

               if( inputParamInfo->flags & TA_IN_PRICE_VOLUME )
               {
                  printIndent( out, indent );
                  if( frame )
                     fprintf( out, "params->in[%d].data.inPrice.volume, /*", paramNb );
                  fprintf( out, "%-*s %s%s",
                         prototype? 12 : 0,
                         prototype? inputIntArrayType : "",
                         "inVolume",
                         prototype? arrayBracket : "" );
                  fprintf( out, "%s\n", frame? " */":"," );
               }

               if( inputParamInfo->flags & TA_IN_PRICE_OPENINTEREST )
               {
                  printIndent( out, indent );
                  if( frame )
                     fprintf( out, "params->in[%d].data.inPrice.openInterest, /*", paramNb );
                  fprintf( out, "%-*s %s%s",
                         prototype? 12 : 0,
                         prototype? inputIntArrayType : "",
                         "inOpenInterest",
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
            printf( "[%s,%s,%d] invalid 'input' type(%d)\n",
                    funcInfo->name, paramName, paramNb,
                    inputParamInfo->type );
            return;
         }

         if( inputParamInfo->type != TA_Input_Price )
         {
            printIndent( out, indent );
            if( validationCode )
               fprintf( out, "if( !%s ) return TA_BAD_PARAM;\n", inputParamInfo->paramName );
            else
            {
               if( frame )
                  fprintf( out, "params->in[%d].data.%s, /*", paramNb, defaultParamName );
               fprintf( out, "%-*s %s%s",
                      prototype? 12 : 0,
                      prototype? typeString : "",                        
                      inputParamInfo->paramName,
                      prototype? arrayBracket : "" );
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
      excludeFromManaged = 0;

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
            excludeFromManaged = 1;
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
         printf( "[%s,%s,%d] invalid 'optional input' type(%d)\n",
                 funcInfo->name, paramName, paramNb,
                 optInputParamInfo->type );
         return;
      }

      if( !paramName )
         paramName = defaultParamName;

      if( validationCode )
      {
         if( excludeFromManaged )
         {
             printIndent( out, indent );
             fprintf( out, "#if !defined(_MANAGED)\n" );
         }

         printOptInputValidation( out, paramName, optInputParamInfo );

         if( excludeFromManaged )
         {
             printIndent( out, indent );
             fprintf( out, "#endif /* !defined(_MANAGED) */\n" );
         }
      }
      else
      {
         if( !(lookbackSignature && (i == 0 )) )
            printIndent( out, indent );

         if( frame )
            fprintf( out, "params->optIn[%d].data.%s, /*", paramNb, defaultParamName );
         fprintf( out, "%-*s %s",
                prototype? 13 : 0,
                prototype? typeString : "",
                paramName );
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
      print( out, "void )%s\n", semiColonNeeded? ";":"" );
   }

   /* Go through all the output */
   if( lookbackSignature )
   {
      print( out, "\n" );
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
            printf( "[%s,%s,%d] invalid 'output' type(%d)\n",
                    funcInfo->name, paramName, paramNb,
                    outputParamInfo->type );
            return;
         }

         if( !paramName )
            paramName = defaultParamName;

         if( validationCode )
         {
            print( out, "   if( %s == NULL )\n", paramName );
            print( out, "      return TA_BAD_PARAM;\n" );
            print( out, "\n" );
         }
         else
         {
            printIndent( out, indent );
            if( frame )
               fprintf( out, "params->out[%d].data.%s%s /*",
                      paramNb, defaultParamName,
                      lastParam? "":"," );

            fprintf( out, "%-*s  %s%s",
                   prototype? 12 : 0,
                   prototype? typeString : "",                     
                   paramName,
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
   genPrefix = 1;
   printFrameHeader( out, funcInfo );
   print( out, "{\n" );
   printFunc( out, "   return ", funcInfo, 0, 1, 1, 0, 0, 0, 0, 0 );
   print( out, "}\n" );
   genPrefix = 0;
}


static void printFrameHeader( FILE *out, const TA_FuncInfo *funcInfo )
{
   print( out, "TA_RetCode TA_%s_FramePP( const TA_ParamHolderPriv *params,\n", funcInfo->name );
   print( out, "                          int            startIdx,\n" );
   print( out, "                          int            endIdx,\n" );
   print( out, "                          int           *outBegIdx,\n" );
   print( out, "                          int           *outNbElement )\n" );
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

   FileHandle *tempFile1;
   
   unsigned int useTempFile;
   FILE *logicIn;
   FILE *logicTmp;
   char localBuf1[500];

   #define TEMPLATE_PASS1   "..\\temp\\pass1.tmp"
   #define TEMPLATE_PASS2   "..\\temp\\pass2.tmp"
   #define TEMPLATE_DEFAULT "..\\src\\ta_abstract\\templates\\ta_x.c.template"
   #define LOGIC_TEMP       "..\\temp\\logic.tmp"

   /* Check if the file already exist. */
   sprintf( localBuf1, "..\\src\\ta_func\\ta_%s.c", funcInfo->name );

   gOutFunc_C = fileOpen( localBuf1, NULL, FILE_READ);
   if( gOutFunc_C == NULL )
      useTempFile = 0;
   else
   {
      useTempFile = 1;
      /* Create a temporary template using it. */
      tempFile1 = fileOpen( TEMPLATE_PASS1, NULL, FILE_WRITE|WRITE_ALWAYS );
      if( tempFile1 == NULL )
      {
         printf( "Cannot create temporary file!\n" );
         return;
      }

      createTemplate( gOutFunc_C, tempFile1 );

      fileClose( tempFile1 );
      fileClose( gOutFunc_C );
   }

   /* Open the file using the template. */
   if( useTempFile )
      gOutFunc_C = fileOpen( TEMPLATE_PASS2, TEMPLATE_PASS1, FILE_WRITE|WRITE_ALWAYS );
   else
      gOutFunc_C = fileOpen( TEMPLATE_PASS2, TEMPLATE_DEFAULT, FILE_WRITE|WRITE_ALWAYS );
      

   if( gOutFunc_C == NULL )
   {
      printf( "Cannot create [%s]\n", localBuf1 );
      return;
   }

   /* Generate. 1st Pass */
   writeFuncFile( funcInfo );

   fileClose( gOutFunc_C );

   if( !useTempFile )
   {
      /* When the file is new, the first pass becomes the 
       * original.
       */
      if( !copyFile( TEMPLATE_PASS2, localBuf1 ) )
      {
         printf( "Cannot copy %s to %s\n", TEMPLATE_PASS2, localBuf1 );
         return;
      }
   }

   /* Extract the TA function code in a temporary file */
   logicIn = fopen( localBuf1, "r" );
   if( !logicIn )
   {
      printf( "Cannot open [%s] for extracting TA logic\n", localBuf1 );
      return;
   }
   logicTmp = fopen( LOGIC_TEMP, "w" );
   if( !logicTmp )
   {
      printf( "Cannot open logic.tmp\n" );
      return;
   }
   extractTALogic( logicIn, logicTmp );
   fclose(logicIn);
   fclose(logicTmp);

   /* Insert the TA function code in the single-precision frame 
    * using the template generated from the first pass.
    */
   gOutFunc_C = fileOpen( localBuf1, TEMPLATE_PASS2, FILE_WRITE|WRITE_ON_CHANGE_ONLY );
   if( gOutFunc_C == NULL )
   {
      printf( "Cannot complete 2nd pass with [%s]\n", localBuf1 );
      return;
   }

   /* Duplicate the function, but using float this time */
   print( gOutFunc_C->file, "\n" );
   print( gOutFunc_C->file, "#define  USE_SINGLE_PRECISION_INPUT\n" );
   print( gOutFunc_C->file, "#if !defined( _MANAGED )\n" );
   print( gOutFunc_C->file, "   #undef   TA_PREFIX\n" );
   print( gOutFunc_C->file, "   #define  TA_PREFIX(x) TA_S_##x\n" );
   print( gOutFunc_C->file, "#endif\n" );
   print( gOutFunc_C->file, "#undef   INPUT_TYPE\n" );
   print( gOutFunc_C->file, "#define  INPUT_TYPE float\n" );
   
   print( gOutFunc_C->file, "#if defined( _MANAGED )\n" );
   printFunc( gOutFunc_C->file, NULL, funcInfo, 1, 0, 0, 0, 0, 1, 0, 1 );
   print( gOutFunc_C->file, "#else\n" );
   printFunc( gOutFunc_C->file, NULL, funcInfo, 1, 0, 0, 0, 0, 0, 0, 1 );
   print( gOutFunc_C->file, "#endif\n" );

   /* Insert the internal logic of the function */
   logicTmp = fopen( LOGIC_TEMP, "r" );
   if( !logicTmp )
   {
      printf( "Cannot read open logic.tmp\n" );
      return;
   }
   while( fgets(gTempBuf,BUFFER_SIZE,logicTmp) )
      fputs( gTempBuf, gOutFunc_C->file );
   fclose(logicTmp);
   print( gOutFunc_C->file, "\n" );

   /* Add the suffix at the end of the file. */
   print( gOutFunc_C->file, "#if defined( _MANAGED )\n" );
   print( gOutFunc_C->file, "}} // Close namespace TA.Lib\n" );
   print( gOutFunc_C->file, "#endif\n" );

   fileClose( gOutFunc_C );
   fileDelete( LOGIC_TEMP );
   fileDelete( TEMPLATE_PASS1 );
   fileDelete( TEMPLATE_PASS2 );
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
   tempFile = fileOpen( FILE_TA_DEFS_TMP, NULL, FILE_WRITE|WRITE_ALWAYS );
   if( tempFile == NULL )
   {
      printf( "Cannot create temporary file!\n" );
      exit(-1);
   }

   createTemplate( gOutDefs_H, tempFile );

   fileClose( tempFile );
   fileClose( gOutDefs_H );

   /* Re-open the file using the template. */
   gOutDefs_H = fileOpen( FILE_TA_DEFS_H, FILE_TA_DEFS_TMP, FILE_WRITE|WRITE_ON_CHANGE_ONLY );
                                                    
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
   fileDelete( FILE_TA_DEFS_TMP );
   #undef FILE_TA_DEFS_H
   #undef FILE_TA_DEFS_TMP
}

static int createProjTemplate( FileHandle *in, FileHandle *out )
{
   FILE *inFile;
   FILE *outFile;
   unsigned int skipSection;
   unsigned int sectionDone;
   unsigned int step;

   inFile = in->file;
   outFile = out->file;

   skipSection = 0;
   sectionDone = 0;
   step        = 0;

   while( fgets( gTempBuf, BUFFER_SIZE, inFile ) )
   {
      if( !skipSection )
      {
         fputs( gTempBuf, outFile );
         if( !strstr( gTempBuf, "<Filter" ) )
            continue;

         if( !fgets( gTempBuf2, BUFFER_SIZE, inFile ) )
         {
            printf( "Unexpected end-of-file\n" );
            return -1;
         }
         fputs( gTempBuf2, outFile );

         if( !strstr( gTempBuf2, "Name=\"ta_func\"" ) )
            continue;            

         if( !fgets( gTempBuf3, BUFFER_SIZE, inFile ) )
         {
            printf( "Unexpected end-of-file\n" );
            return -1;
         }

         fputs( gTempBuf3, outFile );

         if( !strstr( gTempBuf3, "Filter=\"\">" ) )
            continue;            

         skipSection = 1;
         fputs( "%%%GENCODE%%%\n", outFile );
      }
      else if( strstr( gTempBuf, "</Filter>" ) )
      {
         skipSection = 0;
         fputs( gTempBuf, outFile );
         sectionDone++;
      }
   }

   return 0;
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
   while( fgets( gTempBuf, BUFFER_SIZE, inFile ) )
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

   genPrefix = 1;
   print( out, "\n" );
   print( out, "#if defined( _MANAGED )\n" );
   print( out, "   #using <mscorlib.dll>\n" );
   print( out, "   #include \"Core.h\"\n" );
   print( out, "   #define TA_INTERNAL_ERROR(Id) (TA_INTERNAL_ERROR)\n" );
   print( out, "   namespace TA { namespace Lib {\n" );
   print( out, "#else\n" );
   print( out, "   #include <string.h>\n" );
   print( out, "   #include <math.h>\n" );
   print( out, "   #include \"ta_func.h\"\n" );   
   print( out, "   #include \"ta_trace.h\"\n" );
   print( out, "#endif\n" );
   print( out, "\n" );
   print( out, "#ifndef TA_UTILITY_H\n" );
   print( out, "   #include \"ta_utility.h\"\n" );
   print( out, "#endif\n" );
   print( out, "\n" );
   print( out, "#ifndef TA_MEMORY_H\n" );
   print( out, "   #include \"ta_memory.h\"\n" );
   print( out, "#endif\n" );
   print( out, "\n" );
   print( out, "#define TA_PREFIX(x) TA_##x\n" );
   print( out, "#define INPUT_TYPE   double\n" );
   print( out, "\n" );
   print( out, "#if defined( _MANAGED )\n" );
   printFunc( out, NULL, funcInfo, 1, 0, 0, 0, 1, 1, 0, 0 );
   print( out, "#else\n" );
   printFunc( out, NULL, funcInfo, 1, 0, 0, 0, 1, 0, 0, 0 );
   print( out, "#endif\n" );

   genPrefix = 0;
   skipToGenCode( funcInfo->name, gOutFunc_C->file, gOutFunc_C->templateFile );

   genPrefix = 1;
   fprintf( out, "/*\n" );
   printFuncHeaderDoc( out, funcInfo, " * " );
   fprintf( out, " */\n" );
   print( out, "\n" );
   print( out, "#if defined( _MANAGED )\n" );
   printFunc( out, NULL, funcInfo, 1, 0, 0, 0, 0, 1, 0, 0 );
   print( out, "#else\n" );
   printFunc( out, NULL, funcInfo, 1, 0, 0, 0, 0, 0, 0, 0 );
   print( out, "#endif\n" );

   genPrefix = 0;
   skipToGenCode( funcInfo->name, gOutFunc_C->file, gOutFunc_C->templateFile );

   genPrefix = 1;
   print( out, "\n" );
   print( out, "#ifndef TA_FUNC_NO_RANGE_CHECK\n" );
   print( out, "\n" );
   print( out, "   /* Validate the requested output range. */\n" );
   print( out, "   if( startIdx < 0 )\n" );
   print( out, "      return TA_OUT_OF_RANGE_START_INDEX;\n" );
   print( out, "   if( (endIdx < 0) || (endIdx < startIdx))\n" );
   print( out, "      return TA_OUT_OF_RANGE_END_INDEX;\n" );
   print( out, "\n" );
   /* Generate the code for checking the parameters.
    * Also generates the code for setting up the
    * default values.
    */
   print( out, "   /* Validate the parameters. */\n" );
   printFunc( out, NULL, funcInfo, 0, 0, 0, 1, 0, 0, 0, 0 );

   print( out, "#endif /* TA_FUNC_NO_RANGE_CHECK */\n" );
   print( out, "\n" );

   skipToGenCode( funcInfo->name, gOutFunc_C->file, gOutFunc_C->templateFile );

   /* Put a marker who is going to be used in the second pass */
   fprintf( out, "%%%%%%GENCODE%%%%%%\n" );
}

static void printOptInputValidation( FILE *out,
                                     const char *name,                                     
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
      print( out, "   /* min/max are checked for %s. */\n", name );
   case TA_OptInput_RealRange:
      print( out, "   if( %s == TA_REAL_DEFAULT )\n", name  );
      print( out, "      %s = %s;\n", name, doubleToStr(optInputParamInfo->defaultValue) );
      print( out, "   else if( (%s < %s) ||", name, doubleToStr(minReal) );
      print( out, " (%s > %s) )\n", name, doubleToStr(maxReal) );              
      break;
   case TA_OptInput_IntegerRange:
      print( out, "   /* min/max are checked for %s. */\n", name );
   case TA_OptInput_IntegerList:
      print( out, "   if( (int)%s == TA_INTEGER_DEFAULT )\n", name );
      print( out, "      %s = %d;\n", name, (int)optInputParamInfo->defaultValue );
      print( out, "   else if( ((int)%s < %d) || ((int)%s > %d) )\n",
              name, minInt,
              name, maxInt );
      break;
   }

   print( out, "      return TA_BAD_PARAM;\n" );
   print( out, "\n" );
}


static int skipToGenCode( const char *dstName, FILE *out, FILE *templateFile )
{
   unsigned int headerWritten = 0;

   while( fgets( gTempBuf, BUFFER_SIZE, templateFile ) )
   {
      if( strncmp( gTempBuf, "%%%GENCODE%%%", 13 ) == 0 )
      {
         headerWritten = 1;
         break;
      }
      if( out && (fputs( gTempBuf, out ) == EOF) )
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

   fprintf( out, "\n" );
   fprintf( out, "%sInput  = ", prefix );
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
            printf( "[%s,%s,%d] invalid 'optional input' type(%d)\n",
                    funcInfo->name, paramName, paramNb,
                    optInputParamInfo->type );
            return;
         }

         if( !paramName )
            paramName = defaultParamName;

         fprintf( out, "%s%s:", prefix, paramName );
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
   fprintf( out, "__value enum TA_FuncUnstId\n");
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
                             FILE_WRITE|WRITE_ON_CHANGE_ONLY );

   if( gOutRetCode_C == NULL )
   {
      printf( "\nCannot access [%s]\n", gToOpen );
      return -1;
   }

   /* Create "ta_retcode.csv" */
   gOutRetCode_CSV = fileOpen( "..\\src\\ta_common\\ta_retcode.csv",
                               NULL,
                               FILE_WRITE|WRITE_ON_CHANGE_ONLY );

   if( gOutRetCode_CSV == NULL )
   {
      fileClose( gOutRetCode_C );
      printf( "\nCannot access [%s]\n", gToOpen );
      return -1;
   }

   inHdr = fileOpen( "..\\include\\ta_defs.h",
                     NULL,
                     FILE_READ );
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
   fprintf( out, "__value enum TA_MAType\n" );
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

static void extractTALogic( FILE *inFile, FILE *outFile )
{
   int i, length, nbCodeChar;
   int commentBlock, commentFirstCharFound, outIdx;

   #define START_DELIMITATOR "/**** END GENCODE SECTION 2"
   #define STOP_DELIMITATOR  "/**** START GENCODE SECTION 4"

   /* Find the begining of the function */
   while( fgets( gTempBuf, BUFFER_SIZE, inFile ) )
   {
      length = strlen(gTempBuf);
      if( length > BUFFER_SIZE )
         return;

      if( strncmp( gTempBuf, START_DELIMITATOR, strlen(START_DELIMITATOR) ) == 0)
         break;
   }

   /* Copy until the end of the function.
    * At the same time, eliminate all empty line and
    * comments to avoid confusion with original code.
    */
   commentBlock = 0;
   commentFirstCharFound = 0;
   while( fgets( gTempBuf, BUFFER_SIZE, inFile ) )
   {
      length = strlen(gTempBuf);
      if( length > BUFFER_SIZE )
         return;

      if( strncmp( gTempBuf, STOP_DELIMITATOR, strlen(STOP_DELIMITATOR) ) == 0)
         break;

      /* Process the whole line and put it in gTempBuf2 */
      outIdx = 0;
      nbCodeChar = 0;
      for( i=0; i < length; i++ )
      {
         if( !commentBlock )
         {
            if( !commentFirstCharFound )
            {
               if( gTempBuf[i] == '/' )
                  commentFirstCharFound = 1;
               else
               {
                  gTempBuf2[outIdx++] = gTempBuf[i];
                  if( !isspace(gTempBuf[i]) )
                     nbCodeChar++;
               }
            }
            else
            {
               commentFirstCharFound = 0;
               if( gTempBuf[i] == '*' )
                  commentBlock = 1;
               else 
               {
                  gTempBuf2[outIdx++] = '/';
                  nbCodeChar++;
                  if( gTempBuf[i] == '/' )
                     commentFirstCharFound = 1;
                  else
                  {
                     gTempBuf2[outIdx++] = gTempBuf[i];
                     if( !isspace(gTempBuf[i]) )
                        nbCodeChar++;
                  }
               }
            }            
         }
         else
         {
            if( !commentFirstCharFound )
            {
               if( gTempBuf[i] == '*' )
                  commentFirstCharFound = 1;
            }
            else
            {
               commentFirstCharFound = 0;
               if( gTempBuf[i] == '/' )
                  commentBlock = 0;
               else if( gTempBuf[i] == '*' )
                  commentFirstCharFound = 1;
            }            
         }
      }

      if( nbCodeChar != 0 )
      {
         gTempBuf2[outIdx] = '\0';
         fputs( "/* Generated */ ", outFile );
         fputs( gTempBuf2, outFile );
      }
   }
}

static void sortFile( const char *file )
{
   strcpy( gTempBuf, "sort " );
   strcat( gTempBuf, file );
   strcat( gTempBuf, " >" );
   strcat( gTempBuf, file );
   strcat( gTempBuf, ".tmp" );
   system( gTempBuf );
    
   strcpy( gTempBuf, file );
   strcat( gTempBuf, ".tmp" );
   copyFile( gTempBuf, file );
}

static int copyFile( const char *src, const char *dest )
{
   FILE *in;
   FILE *out;

   in = fopen( src, "rb" );
   if( !in )
      return 0;

   out = fopen( dest, "wb" );
   if( !out )
   {
      fclose( in );
      return 0;
   }

   while( fgets( gTempBuf, BUFFER_SIZE, in ) )
   {
      fputs(gTempBuf,out);
   }

   fclose(in);
   fclose(out);
   return 1;
}

static int areFileSame( const char *file1, const char *file2 )
{
   /* Text comparison of both files */
   unsigned int i;

   FILE *f1;
   FILE *f2;

   f1 = fopen( file1, "r" );
   if( !f1 )
      return 0;

   f2 = fopen( file2, "r" );
   if( !f2 )
   {
      fclose( f1 );
      return 0;
   }
   
   memset( gTempBuf,  0, sizeof(gTempBuf ) );
   memset( gTempBuf2, 0, sizeof(gTempBuf2) );

   while( fgets( gTempBuf, BUFFER_SIZE, f1 ) )
   {
      if( !fgets( gTempBuf2, BUFFER_SIZE, f2 ) )
      {
         fclose(f1);
         fclose(f2);
         return 0;
      } 
      
      for( i=0; i < sizeof(gTempBuf); i++ )
      {
         if( gTempBuf[i] != gTempBuf2[i] )
         {
            fclose(f1);
            fclose(f2);
            return 0;
         }          
         if( gTempBuf[i] == '\0' )
            i = sizeof(gTempBuf);
      }

      memset( gTempBuf,  0, sizeof(gTempBuf ) );
      memset( gTempBuf2, 0, sizeof(gTempBuf2) );
   }

   if( fgets( gTempBuf2, BUFFER_SIZE, f2 ) )
   {
      fclose(f1);
      fclose(f2);
      return 0;
   }
   
   fclose(f1);
   fclose(f2);
   return 1;
}
