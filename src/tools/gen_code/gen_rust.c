/** RUST functions that get injected hackily into the bottom gen_code.c **/

static void printRustLookbackFunctionSignature(FILE* out,
                                               const char* prefix, /* Can be NULL */
                                               const TA_FuncInfo* funcInfo)
{
   char funcNameBuffer[1024]; /* Not safe, but 1024 is realistic, */
   toLowerSnakeCase(funcInfo->name, funcNameBuffer);

   // print lookback function header
   sprintf(gTempBuf, "%sfn %s_lookback( ",
           prefix? prefix:"",
           funcNameBuffer);

   // TODO: print input params
   // TODO: print optional input params
   // TODO: close function
   // TODO: print return type
   print(out, gTempBuf);
   print(out, "\n");
}

static void printRustDoublePrecisionFunctionSignature(FILE* out,
                                                      const char* prefix, /* Can be NULL */
                                                      const TA_FuncInfo* funcInfo)
{
   char funcNameBuffer[1024]; /* Not safe, but 1024 is realistic, */
   const TA_OptInputParameterInfo *optInputParamInfo;
   const TA_OutputParameterInfo *outputParamInfo;
   const TA_InputParameterInfo *inputParamInfo;
   toLowerSnakeCase(funcInfo->name, funcNameBuffer);
   const char* inputDoubleArrayType = "double";
   const char* inputIntArrayType = "int";
   const char* outputDoubleArrayType = "double";
   const char* outputIntArrayType = "int";
   const char* outputIntParam = "int";
   const char* arrayBracket = "[]";
   const char* startIdxString = "startIdx";
   const char* endIdxString = "endIdx";
   const char* outNbElementString = "outNBElement";
   const char* outBegIdxString = "outBegIdx";
   const char* funcName = funcInfo->name;
   const char *defaultParamName;
   const char *typeString;
   int indent, i;

   sprintf(gTempBuf, "%sfn %s( int    %s,\n",
           prefix? prefix:"",
           funcNameBuffer,
           startIdxString);

   indent = (unsigned int)strlen(gTempBuf);

   // TODO: print input params
   // TODO: print optional input params
   // TODO: close function
   // TODO: print return type
   print(out, gTempBuf);
   print(out, "\n");
   printIndent(out, indent);
   fprintf(out, "int    %s,\n", endIdxString);

   for (i = 0; i < funcInfo->nbInput; i++)
   {
      // retCode = TA_GetInputParameterInfo( funcInfo->handle, i, &inputParamInfo );
      TA_GetInputParameterInfo(funcInfo->handle, i, &inputParamInfo);

      // if( retCode != TA_SUCCESS )
      // {
      //    printf( "[%s] invalid 'input' information (%d,%d)\n", funcName, i, paramNb );
      //    return;
      // }


      switch (inputParamInfo->type)
      {
      case TA_Input_Real:
         typeString = inputDoubleArrayType;
         defaultParamName = "inReal";
         break;
      }


      // for every input param just do this:
      fprintf(out, "%-*s %s%s",
                           0,
                           typeString,
                           inputParamInfo->paramName,
                           arrayBracket);
      fprintf(out, ",\n");

   }

      // optional inputs
   for( i=0; i < funcInfo->nbOptInput; i++ )
   {
      TA_GetOptInputParameterInfo( funcInfo->handle, i, &optInputParamInfo );

      switch( optInputParamInfo->type )
      {
      case TA_OptInput_RealRange:
         typeString = "double";
         defaultParamName = "optInReal";
         break;
      }

      // for every output param just do this:
      fprintf(out, "%-*s %s%s",
                           0,
                           typeString,
                           optInputParamInfo->paramName,
                           arrayBracket);
      fprintf(out, ",\n");
   }

      // outputs
      // TODO: outBegIdx should probably be borrowed
   fprintf(out, "mut outBegIdx,\n");
      // TODO: outNBElement should probably be borrowed
   fprintf(out, "mut outNBElement,\n");

   for (i = 0; i < funcInfo->nbOutput; i++)
   {
      // retCode =  TA_GetOutputParameterInfo(funcInfo->handle, i, &outputParamInfo);
      TA_GetOutputParameterInfo(funcInfo->handle, i, &outputParamInfo);
      switch (outputParamInfo->type)
      {
      case TA_Output_Real:
         typeString = outputDoubleArrayType;
         defaultParamName = "outReal";
         break;
      }

      // for every output param just do this:
      fprintf(out, "%-*s %s%s",
                           0,
                           typeString,
                           outputParamInfo->paramName,
                           arrayBracket);
      fprintf(out, ",\n");
   }

   // todo: remove the extra comma and newline from out
  fprintf(out, ")\n");


}

static void printRustSinglePrecisionFunctionSignature(FILE* out,
                                                      const char* prefix, /* Can be NULL */
                                                      const TA_FuncInfo* funcInfo)
{
   const char* inputDoubleArrayType = "const float";
   const char* inputIntArrayType = "const int";
   const char* outputDoubleArrayType = "double";
   const char* outputIntArrayType = "int";
   const char* outputIntParam = "int";
   const char* arrayBracket = "[]";
   const char* startIdxString = "startIdx";
   const char* endIdxString = "endIdx";
   const char* outNbElementString = "outNBElement";
   const char* outBegIdxString = "outBegIdx";
   const char* funcName = funcInfo->name;
   int indent;

   char funcNameBuffer[1024]; /* Not safe, but 1024 is realistic, */
   toLowerSnakeCase(funcInfo->name, funcNameBuffer);

   sprintf(gTempBuf, "%sfn %s_s( int    %s,\n",
           prefix? prefix:"",
           funcNameBuffer,
           startIdxString);
   indent = (unsigned int)strlen(gTempBuf);


   // TODO: print input params
   // TODO: print optional input params
   // TODO: close function
   // TODO: print return type
   // TODO: handle validation logic
   // TODO: handle abstract frame logic
   print(out, gTempBuf);
   print(out, "\n");
   printIndent(out, indent);
   fprintf(out, "int    %s,\n", endIdxString);
}

void writeRustModLines(const TA_FuncInfo* funcInfo, void* opaque)
{
   struct WriteRustModLinesParams* params = (struct WriteRustModLinesParams*)opaque;
   FileHandle* out = params->out;
   char buffer[500];

#if defined(RUST_SINGLE_FUNC)
   if (strcmp(funcInfo->name,RUST_SINGLE_FUNC) != 0)
      return;
#endif

   // Convert filename to lowercase into buffer.
   int i = 0;
   while (funcInfo->name[i])
   {
      buffer[i] = tolower(funcInfo->name[i]);
      i++;
   }
   buffer[i] = '\0';

   if (params->writePubUse == 1)
      fprintf(out->file, "pub use self::%s::*\n", buffer);
   else
      fprintf(out->file, "pub mod %s;\n", buffer);
}

void writeRustMod(void)
{
   // Update the rust/src/ta_func/mod.rs file.
   struct WriteRustModLinesParams params;
   char buffer[500];
   FileHandle* out;

   if (!gmcpp_installed)
      return;

   // Add rs file to ta_func/mod.rs
   // A common header/footer is provided by the template file.
#define FILE_RUST_MOD ".." PATH_SEPARATOR "rust" PATH_SEPARATOR "src" PATH_SEPARATOR "ta_func" PATH_SEPARATOR "mod.rs"
#define FILE_RUST_MOD_TEMPLATE ".." PATH_SEPARATOR "src" PATH_SEPARATOR "ta_abstract" PATH_SEPARATOR "templates" PATH_SEPARATOR "ta_func_mod.rs.template"

   out = fileOpen(FILE_RUST_MOD,
                  FILE_RUST_MOD_TEMPLATE,
                  FILE_WRITE | WRITE_ON_CHANGE_ONLY);


   params.out = out;
   params.writePubUse = 1;
   TA_ForEachFunc(writeRustModLines, &params);
   params.writePubUse = 0;
   TA_ForEachFunc(writeRustModLines, &params);

   fileClose(out);
}

void genRustCodePhase2(const TA_FuncInfo* funcInfo)
{
   // Each TA function get its own .rs file generated.
   // A common header/footer is provided by the template file.
   FILE* logicTmp;
   char buffer[500];
   int idx, again;
   static int firstTime = 1;
   int ret;

#if defined(RUST_SINGLE_FUNC)
   if (strcmp(funcInfo->name,RUST_SINGLE_FUNC) != 0)
      return;
#endif

   if (!gmcpp_installed)
      return;


   // Convert filename to lowercase into buffer.
   int i = 0;
   while (funcInfo->name[i])
   {
      buffer[i] = tolower(funcInfo->name[i]);
      i++;
   }
   buffer[i] = '\0';
   strcat(buffer, ".rs");
#define FILE_RUST_FUNC_TEMPLATE ".." PATH_SEPARATOR "src" PATH_SEPARATOR "ta_abstract" PATH_SEPARATOR "templates" PATH_SEPARATOR "ta_x.rs.template"

   FileHandle* out = fileOpen(ta_fs_path(5, "..", "rust", "src", "ta_func", buffer),
                              FILE_RUST_FUNC_TEMPLATE,
                              FILE_WRITE | WRITE_ON_CHANGE_ONLY);


   /* Clean-up just in case. */
   fileDelete(ta_fs_path(3, "..", "temp", "rust_logic.tmp"));

#ifdef _MSC_VER
     sprintf( buffer, "%s -c -+ -z -P -I.." PATH_SEPARATOR "src" PATH_SEPARATOR "ta_common -I.." PATH_SEPARATOR "src" PATH_SEPARATOR "ta_abstract -I.." PATH_SEPARATOR "include -D _RUST .." PATH_SEPARATOR "src" PATH_SEPARATOR "ta_func" PATH_SEPARATOR "TA_%s.c >>.." PATH_SEPARATOR "temp" PATH_SEPARATOR "rust_logic.tmp ", gmcpp_exec, funcInfo->name);
     ret = system( buffer );
#else
   sprintf(buffer,
           "%s -@compat -+ -z -P -I.." PATH_SEPARATOR "src" PATH_SEPARATOR "ta_common -I.." PATH_SEPARATOR "src"
           PATH_SEPARATOR "ta_abstract -I.." PATH_SEPARATOR "include -D _RUST .." PATH_SEPARATOR "src" PATH_SEPARATOR
           "ta_func" PATH_SEPARATOR "ta_%s.c | sed '/^#include/d' >> .." PATH_SEPARATOR "temp" PATH_SEPARATOR
           "rust_logic.tmp ", gmcpp_exec, funcInfo->name);
   ret = system(buffer);
#endif

   /* Write the output of the C pre-processor to the rust file. */
   init_gToOpen(ta_fs_path(3, "..", "temp", "rust_logic.tmp"), NULL);
   logicTmp = fopen(gToOpen, "r");
   if (!logicTmp)
   {
      printf("Cannot open temp/rust_logic.tmp\n");
      return;
   }
   while (fgets(gTempBuf,BUFFER_SIZE, logicTmp))
   {
      /* Remove empty lines and lines with only a ';' */
      idx = 0;
      again = 1;
      while (again && gTempBuf[idx] != '\0')
      {
         if (!isspace(gTempBuf[idx]) && !(gTempBuf[idx] == ';'))
            again = 0;
         idx++;
      }
      if ((again == 0) && (idx > 0))
         fputs(gTempBuf, out->file);
   }

   /* Clean-up */
   fclose(logicTmp);
   print(out->file, "\n");
   fileDelete(ta_fs_path(3, "..", "temp", "rust_logic.tmp"));

   // Upon closing, will touch the target file only if there was a change...
   fileClose(out);
}

